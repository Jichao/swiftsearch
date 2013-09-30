#include "stdafx.h"
#include "targetver.h"

#include <time.h>

#include "NtfsIndex.hpp"

#include <map>
#include <vector>

#include <boost/range/iterator_range.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_circular_buffer.hpp>

#include "NTFS (1).h"
#include "CComCritSecLock.hpp"
#include "NtObjectMini.hpp"

struct first_less { template<class P1, class P2> bool operator()(P1 const &a, P2 const &b) const { return a.first < b.first; } };

NtfsIndex::NtfsIndex(
	winnt::NtFile &volume, std::basic_string<TCHAR> const &win32Path, winnt::NtObject &is_allowed_event,
	unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, unsigned long volatile *const pSpeed, bool volatile *pBackground)
	: _win32Path(win32Path)
{
	unsigned long const clusterSize = volume.GetClusterSize();

	unsigned long long total_mft_size = 0;
	size_t file_record_size;
	std::vector<std::pair<unsigned long long, unsigned long long> > mft_extents;
	{
		winnt::NtFile mft = winnt::NtFile::NtOpenFile(
			winnt::ObjectAttributes(0x0001000000000000, volume.get()),
			winnt::Access::QueryAttributes | winnt::Access::Synchronize,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID);
		file_record_size = mft.FsctlGetVolumeData().BytesPerFileRecordSegment;
		std::vector<unsigned char> buf(sizeof(RETRIEVAL_POINTERS_BUFFER));
		BOOL success;
		for (;;)
		{
			size_t cb = buf.size();
			success = mft.FsctlGetRetrievalPointers(reinterpret_cast<PRETRIEVAL_POINTERS_BUFFER>(&buf[0]), cb, 0);
			if (!success) { buf.resize(buf.size() * 2); }
			else { break; }
		}
		RETRIEVAL_POINTERS_BUFFER const &retPtrsBuf = *reinterpret_cast<PRETRIEVAL_POINTERS_BUFFER>(&buf[0]);
		mft_extents.reserve(retPtrsBuf.ExtentCount);
		for (size_t i = 0; i < retPtrsBuf.ExtentCount; i++)
		{
			std::pair<unsigned long long, unsigned long long> extent(
				retPtrsBuf.Extents[i].Lcn.QuadPart,
				retPtrsBuf.Extents[i].NextVcn.QuadPart -
				(i > 0
					? retPtrsBuf.Extents[i - 1].NextVcn.QuadPart
					: retPtrsBuf.StartingVcn.QuadPart));
			extent.first *= clusterSize;
			extent.second *= clusterSize;
			total_mft_size += extent.second;
			mft_extents.push_back(extent);
		}
	}
	
	struct ProgressReporter
	{
		winnt::NtObject *is_allowed_event;
		unsigned long old_progress;
		unsigned long volatile *const pProgress;
		unsigned long volatile *const pSpeed;
		clock_t old_time;
		bool volatile *pBackground;
		bool background_old;
		size_t counter;
		ProgressReporter(winnt::NtObject &is_allowed_event, unsigned long volatile *const pProgress, unsigned long volatile *const pSpeed, bool volatile *pBackground)
			: is_allowed_event(&is_allowed_event), pProgress(pProgress), old_progress(), pSpeed(pSpeed), pBackground(pBackground), counter(0), background_old(), old_time(clock()) { }
		void operator()(unsigned long long const numerator, unsigned long long const denominator)
		{
			if (pBackground && *pBackground != background_old)
			{
				background_old = *pBackground;
				SetThreadPriority(GetCurrentThread(), background_old ? THREAD_PRIORITY_BELOW_NORMAL : THREAD_PRIORITY_NORMAL);
			}
			clock_t const now = clock();
			unsigned long const new_progress = static_cast<unsigned long>(PROGRESS_CANCEL_REQUESTED * numerator / (denominator == 0 ? 1 : denominator));
			if (pSpeed)
			{
				*pSpeed = static_cast<unsigned long>(static_cast<unsigned long long>(new_progress - old_progress) * CLOCKS_PER_SEC / max(1U, now - this->old_time));
			}
			if (pProgress)
			{
				this->old_progress = InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), new_progress);
				if (this->old_progress == static_cast<long>(PROGRESS_CANCEL_REQUESTED))
				{
					InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), PROGRESS_CANCEL_REQUESTED);  // Put back a zero again
					throw CStructured_Exception(ERROR_CANCELLED, NULL);
				}
			}
			this->old_time = now;
		}
	};
	ProgressReporter progressReporter(is_allowed_event, pProgress, pSpeed, pBackground);

	size_t const nFileRecords = static_cast<size_t>(total_mft_size / file_record_size);
	this->index.reserve(nFileRecords * 4);
	this->names.reserve(nFileRecords * 12);

	StandardInfoAttrs standardInfoAttrs; standardInfoAttrs.reserve(nFileRecords * 4);
	FileNameAttrs fileNameAttrs; fileNameAttrs.reserve(nFileRecords * 4);
	DataAttrs dataAttrs; dataAttrs.reserve(nFileRecords * 4);
	FileNameAttrsDerived fileNameAttrsDerived; fileNameAttrsDerived.reserve(nFileRecords * 4);
	DataAttrsDerived dataAttrsDerived; dataAttrsDerived.reserve(nFileRecords * 4);
	Children children; children.reserve(nFileRecords * 4);
	{
		struct Callback
		{
			NtfsIndex *me;
			StandardInfoAttrs *standardInfoAttrs;
			FileNameAttrs *fileNameAttrs;
			DataAttrs *dataAttrs;
			FileNameAttrsDerived *fileNameAttrsDerived;
			DataAttrsDerived *dataAttrsDerived;
			Children *children;
			unsigned long clusterSize;
			size_t cbFileRecord, nFileRecords;
			ProgressReporter *progressReporter;
			SegmentNumber next_segment_number_to_process;

			Callback(
				NtfsIndex *me,
				StandardInfoAttrs &standardInfoAttrs, FileNameAttrs &fileNameAttrs, DataAttrs &dataAttrs,
				FileNameAttrsDerived &fileNameAttrsDerived, DataAttrsDerived &dataAttrsDerived,
				Children &children, unsigned long clusterSize, size_t cbFileRecord, size_t nFileRecords, ProgressReporter &progressReporter
			) :
				me(me),
				standardInfoAttrs(&standardInfoAttrs), fileNameAttrs(&fileNameAttrs), dataAttrs(&dataAttrs),
				fileNameAttrsDerived(&fileNameAttrsDerived), dataAttrsDerived(&dataAttrsDerived),
				children(&children), clusterSize(clusterSize), cbFileRecord(cbFileRecord),
				nFileRecords(nFileRecords), progressReporter(&progressReporter), next_segment_number_to_process(0) { }

			void operator()(unsigned long long offset, void *buffer, size_t bytes)
			{
				SegmentNumber segmentNumber = static_cast<SegmentNumber>(offset / cbFileRecord);
				for (unsigned char *p = static_cast<unsigned char *>(buffer);
					p < static_cast<unsigned char *>(buffer) + bytes;
					p += cbFileRecord, ++segmentNumber)
				{
					NTFS::FILE_RECORD_SEGMENT_HEADER *const pRecord =
						reinterpret_cast<NTFS::FILE_RECORD_SEGMENT_HEADER *>(p);
					(*this)(segmentNumber, pRecord);
				}
			}
			void operator()(SegmentNumber const segmentNumber, NTFS::FILE_RECORD_SEGMENT_HEADER *const pRecord)
			{
				(*progressReporter)(nFileRecords * 0 + segmentNumber * 2, nFileRecords * 4);
				if (pRecord != NULL && pRecord->MultiSectorHeader.TryUnFixup(pRecord->BytesInUse)
					// && (pRecord->Flags & FRH_IN_USE) != 0   // TODO: Do we want deleted files too?
					)
				{
					NTFS::FILE_RECORD_SEGMENT_HEADER const &record = *pRecord;
					bool const isBase = !record.BaseFileRecordSegment;
					SegmentNumber const baseSegment = static_cast<SegmentNumber>(isBase ? segmentNumber : record.BaseFileRecordSegment);
					for (NTFS::ATTRIBUTE_RECORD_HEADER const *ah = record.TryGetAttributeHeader();
						ah != NULL &&
						ah->Length > 0 &&
						ah->TryGetNextAttributeHeader() <= static_cast<void const *>(reinterpret_cast<unsigned char const *>(pRecord) + pRecord->BytesAllocated);

						ah = ah->TryGetNextAttributeHeader())
					{{
						static TCHAR const I30[] = { _T('$'), _T('I'), _T('3'), _T('0') };
						static TCHAR const BAD[] = { _T('$'), _T('B'), _T('a'), _T('d') };
						bool const isI30 = ah->NameLength == sizeof(I30) / sizeof(*I30) && memcmp(I30, ah->GetName(), ah->NameLength) == 0;
						bool const isBad = ah->NameLength == sizeof(BAD) / sizeof(*BAD) && memcmp(BAD, ah->GetName(), ah->NameLength) == 0;
						long long sizeOnDisk = 0;
						if (ah->IsNonResident)
						{
							LONGLONG currentLcn = 0;
							LONGLONG nextVcn = ah->NonResident.LowestVCN;
							LONGLONG currentVcn = nextVcn;
							for (unsigned char const *pRun = ah->NonResident.GetMappingPairs(); *pRun != 0; pRun += NTFS::RunLength(pRun))
							{
								LONGLONG const runCount = NTFS::RunCount(pRun);
								nextVcn += runCount;
								LONGLONG const deltaLCN = NTFS::RunDeltaLCN(pRun);
								if (!(deltaLCN == 0 && (ah->Flags || segmentNumber == 0x000000000008 /* $BadClus */ && isBad && ah->Type == NTFS::AttributeData)))
								{ sizeOnDisk += runCount * clusterSize; }
								currentLcn += deltaLCN;
								currentVcn = nextVcn;
							}
						}
						if (ah->Type == NTFS::AttributeStandardInformation)
						{
							NTFS::STANDARD_INFORMATION const &a = *static_cast<NTFS::STANDARD_INFORMATION const *>(ah->Resident.GetValue());
							if (isBase)
							{
								standardInfoAttrs->push_back(StandardInfoAttrs::value_type(
									baseSegment,
									StandardInfoAttrs::value_type::second_type(
										StandardInfoAttrs::value_type::second_type::first_type(
											a.CreationTime,
											a.LastModificationTime),
										StandardInfoAttrs::value_type::second_type::second_type(
											a.LastAccessTime,
											(a.FileAttributes & ~0x40000000) |
											((record.Flags & FRH_IN_USE) == 0 ? 0x40000000 : 0) |
											((record.Flags & FRH_DIRECTORY) != 0 ? FILE_ATTRIBUTE_DIRECTORY : 0)))));
							}
							else { /* should never happen */ }
						}
						if (ah->Type == NTFS::AttributeFileName)
						{
							NTFS::FILENAME_INFORMATION const &a = *static_cast<NTFS::FILENAME_INFORMATION const *>(ah->Resident.GetValue());
							if (a.Flags != 2)
							{
								if (me->names.size() + a.FileNameLength > me->names.capacity())
								{
									/* Old MSVCRT doesn't do this! */
									me->names.reserve(me->names.size() * 2);
								}
								SegmentNumber const parentSegmentNumber = static_cast<SegmentNumber>(a.ParentDirectory & 0x0000FFFFFFFFFFFF);
								FileNameAttrs::value_type const v(
									baseSegment,
									FileNameAttrs::value_type::second_type(
										FileNameAttrs::value_type::second_type::first_type(
											static_cast<NameOffset>(me->names.size()),
											static_cast<NameLength>(a.FileNameLength)),
										parentSegmentNumber));
								if (isBase) { fileNameAttrs->push_back(v); }
								else
								{
									if (baseSegment >= fileNameAttrsDerived->size())
									{
										fileNameAttrsDerived->resize(baseSegment + 1);
									}
									(*fileNameAttrsDerived)[baseSegment].push_back(v.second);
								}
								me->names.append(a.FileName, a.FileNameLength);
								if (parentSegmentNumber >= children->size())
								{ children->resize(parentSegmentNumber + 1); }
								(*children)[parentSegmentNumber].push_back(baseSegment);
							}
						}
						if (ah->Type != NTFS::AttributeIndexRoot || (static_cast<NTFS::INDEX_ROOT const *>(ah->Resident.GetValue())->Header.Flags & 1) == 0)
						{
							size_t const cchNameOld = me->names.size();
							if (!(ah->NameLength == 0 && ah->Type == NTFS::AttributeData ||
								isI30 && (ah->Type == NTFS::AttributeIndexRoot || ah->Type == NTFS::AttributeIndexAllocation)))
							{
								if (me->names.size() + 1 /* ':' */ + ah->NameLength + 64 /* max attribute name length */ > me->names.capacity())
								{
									/* Old MSVCRT doesn't do this! */
									me->names.reserve((me->names.size() + MAX_PATH) * 2);
								}
								me->names.append(ah->GetName(), ah->NameLength);
							}
							DataAttrs::value_type const v(
								baseSegment,
								DataAttrs::value_type::second_type(
									DataAttrs::value_type::second_type::first_type(ah->Type, (pRecord->Flags & FRH_IN_USE) != 0),
									DataAttrs::value_type::second_type::second_type(
										DataAttrs::value_type::second_type::second_type::first_type(
											static_cast<NameOffset>(cchNameOld),
											static_cast<NameLength>(me->names.size() - cchNameOld)),
										DataAttrs::value_type::second_type::second_type::second_type(
											ah->IsNonResident
											? (
												(baseSegment == 0x000000000008 /* $BadClus */)
												? sizeOnDisk
												: ah->NonResident.DataSize)
											: ah->Resident.ValueLength,
											sizeOnDisk))));
							
							class DupeChecker
							{
								DupeChecker &operator =(DupeChecker const &) { throw std::logic_error(""); }
								std::basic_string<TCHAR> const &names;
								NTFS::ATTRIBUTE_RECORD_HEADER const &ah;
								DataAttrs::value_type const &v;
							public:
								DupeChecker(std::basic_string<TCHAR> const &names, NTFS::ATTRIBUTE_RECORD_HEADER const &ah, DataAttrs::value_type const &v)
									: names(names), ah(ah), v(v) { }
								bool operator()(std::pair<DataAttrs::value_type::first_type const, DataAttrs::value_type::second_type> &p)
								{ return (*this)(p.second); }
								bool operator()(DataAttrs::value_type &p)
								{ return (*this)(p.second); }
								bool operator()(DataAttrs::value_type::second_type &attr)
								{
									bool found = false;
									if (attr.first.first == ah.Type &&
										boost::equal(
											SubTString(ah.GetName(), ah.GetName() + ah.NameLength),
											SubTString(
												names.data() + attr.second.first.first,
												names.data() + attr.second.first.first + attr.second.first.second)))
									{
										if (ah.NonResident.LowestVCN == 0)
										{
											long long const oldSizeOnDisk = attr.second.second.second;
											attr = v.second;
											attr.second.second.second += oldSizeOnDisk;
										}
										else { attr.second.second.second += v.second.second.second.second; }
										found = true;
									}
									return found;
								}
							} dupeChecker(me->names, *ah, v);
							bool include = !ah->IsNonResident;
							if (!include)
							{
								DataAttrs::iterator const lb = std::lower_bound(
									dataAttrs->begin(),
									dataAttrs->end(),
									DataAttrs::value_type(
										baseSegment,
										DataAttrs::value_type::second_type()),
									first_less());
								include = true;
								for (DataAttrs::iterator
									j = lb; j != dataAttrs->end() && j->first == lb->first; ++j)
								{
									if (dupeChecker(*j))
									{
										include = false;
										break;
									}
								}
							}
							if (include)
							{
								if (baseSegment < dataAttrsDerived->size())
								{
									DataAttrsDerived::value_type &v = (*dataAttrsDerived)[baseSegment];
									include &= std::find_if(v.begin(), v.end(), dupeChecker) == v.end();
								}
							}
							if (include)
							{
								if (isBase) { dataAttrs->push_back(v); }
								else
								{
									if (baseSegment >= dataAttrsDerived->size())
									{ dataAttrsDerived->resize(baseSegment + 1); }
									(*dataAttrsDerived)[baseSegment].push_back(v.second);
								}
							}
						}
					}}
				}
			}
		};
		Callback callback(
			this,
			standardInfoAttrs, fileNameAttrs, dataAttrs,
			fileNameAttrsDerived, dataAttrsDerived,
			children, clusterSize, file_record_size, nFileRecords, progressReporter);
		{
			class Buffer
			{
				void *p;
				size_t cb;
			public:
				~Buffer() { operator delete(p); }
				explicit Buffer(size_t const cb = 0) : cb(cb), p(cb ? operator new(cb) : NULL) { }
				Buffer(Buffer const &other)
					: p(other.p ? operator new(other.cb) : NULL), cb(other.cb)
				{
					std::copy(static_cast<unsigned char const *>(other.p), static_cast<unsigned char const *>(other.p) + other.cb, static_cast<unsigned char *>(this->p));
				}
				Buffer &operator =(Buffer const &other)
				{
					Buffer(other).swap(*this);
					return *this;
				}
				void swap(Buffer &other)
				{
					using std::swap;
					swap(this->p, other.p);
					swap(this->cb, other.cb);
				}
				unsigned char *begin() { return static_cast<unsigned char *>(this->p); }
				unsigned char *end() { return static_cast<unsigned char *>(this->p) + static_cast<ptrdiff_t>(this->cb); }
				void *get() const { return this->p; }
				size_t size() const { return this->cb; }
			};

			class OverlappedBuffer : public OVERLAPPED, public Buffer
			{
				OverlappedBuffer(OverlappedBuffer const &)
				{ throw std::logic_error(""); }
				OverlappedBuffer &operator =(OverlappedBuffer const &)
				{ throw std::logic_error(""); }
			public:
				OverlappedBuffer(unsigned long long const offset, unsigned long long const virtual_offset, size_t const buffer_size)
					: OVERLAPPED(), Buffer(buffer_size), virtual_offset(virtual_offset)
				{
					this->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
					this->Offset = static_cast<unsigned long>(offset);
					this->OffsetHigh = static_cast<unsigned long>(offset >> (CHAR_BIT * sizeof(this->Offset)));
				}
				~OverlappedBuffer()
				{
					if (this->hEvent)
					{
						CloseHandle(this->hEvent);
						this->hEvent = NULL;
					}
				}
				void swap(OverlappedBuffer &other)
				{
					using std::swap;
					swap(static_cast<OVERLAPPED &>(*this), static_cast<OVERLAPPED &>(other));
					this->Buffer::swap(static_cast<Buffer &>(other));
					swap(this->virtual_offset, other.virtual_offset);
				}
				unsigned long long virtual_offset;
				void offset(unsigned long long const value)
				{
					this->Offset = static_cast<unsigned long>(value);
					this->OffsetHigh = static_cast<unsigned long>(value >> (sizeof(this->Offset) * CHAR_BIT));
				}
				unsigned long long offset() const
				{
					return this->Offset + (static_cast<unsigned long long>(this->OffsetHigh) << (sizeof(this->Offset) * CHAR_BIT));
				}
			};

			struct FinishPendingReads
			{
				typedef boost::ptr_circular_buffer<OverlappedBuffer> PendingBuffers;
				FinishPendingReads(winnt::NtFile &file, PendingBuffers &p)
					: file(file), p(&p) { }
				~FinishPendingReads()
				{
					while (!p->empty())
					{
						DWORD nb;
						GetOverlappedResult(file.get(), p->pop_front().get(), &nb, TRUE);
					}
				}
			private:
				winnt::NtFile /* duplicate handle! */ file;
				PendingBuffers *p;
			};

			FinishPendingReads::PendingBuffers pending(2), unused(pending.capacity());
			FinishPendingReads const finish_pending_reads(volume, pending);
			size_t chunk_size = 1 << 22;

			clock_t const start = clock();
			(void)start;
			unsigned long long virtual_offset = 0;
			for (size_t i = 0; i < mft_extents.size(); i++)
			{
				unsigned long cb;
				for (size_t j = 0; j < mft_extents[i].second; j += cb)
				{
					bool repeat_with_smaller_chunk;
					do
					{
						while (pending.full())
						{
							FinishPendingReads::PendingBuffers::auto_type next(pending.pop_back());
							DWORD br;
							if (!GetOverlappedResult(volume.get(), next.get(), &br, TRUE))
							{ RaiseException(GetLastError(), 0, 0, NULL); }
							callback(next->virtual_offset, next->get(), br);
							unused.push_back(next.release());
						}
						unsigned long long const offset = mft_extents[i].first + j;
						cb = static_cast<unsigned long>(min(chunk_size >> (pBackground && *pBackground ? 4 : 0), mft_extents[i].second - j));

						if (!volume)
						{
							// TODO: Wait for pending I/O
							RaiseException(ERROR_CANCELLED, 0, 0, NULL);
						}

						WaitForSingleObject(is_allowed_event.get(), INFINITE);
						struct ScopedIoPriority
						{
							winnt::NtFile *pVolume;
							IO_PRIORITY_HINT old;
							ScopedIoPriority(winnt::NtFile &volume, bool const lower)
								: pVolume(&volume), old()
							{
								(void)lower;
								if (volume)
								{
									this->old = volume.GetIoPriorityHint();
									pVolume->SetIoPriorityHint(lower ? IoPriorityVeryLow : old);
								}
							}
							~ScopedIoPriority()
							{
								if (pVolume && *pVolume)
								{
									pVolume->SetIoPriorityHint(old);
								}
							}
						};

						ScopedIoPriority const priority(volume, pBackground && *pBackground);
						if (!unused.empty())
						{
							FinishPendingReads::PendingBuffers::auto_type next(unused.pop_back());
							if (cb <= next->size())
							{
								next->offset(offset);
								next->virtual_offset = virtual_offset;
							}
							else
							{
								OverlappedBuffer(offset, virtual_offset, cb).swap(*next);
							}
							pending.push_front(next.release());
						}
						else { pending.push_front(new OverlappedBuffer(offset, virtual_offset, cb)); }
						OverlappedBuffer *const overlapped = &pending.front();
						bool const success = !!ReadFile(volume.get(), overlapped->get(), cb, NULL, overlapped);
						repeat_with_smaller_chunk = !success &&  cb > 64 * 1024 &&
							(GetLastError() == ERROR_NOT_ENOUGH_MEMORY || GetLastError() == ERROR_NOT_ENOUGH_QUOTA);
						if (!repeat_with_smaller_chunk && !success &&
							GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_IO_PENDING /* MUST check this! */)
						{ winnt::NtStatus::ThrowWin32(GetLastError()); }
						virtual_offset += cb;
						if (repeat_with_smaller_chunk) { chunk_size /= 2; }
					} while (repeat_with_smaller_chunk);
				}
			}
			while (!pending.empty())
			{
				FinishPendingReads::PendingBuffers::auto_type const next(pending.pop_back());
				DWORD br;
				if (!GetOverlappedResult(volume.get(), next.get(), &br, TRUE))
				{ throw CStructured_Exception(GetLastError(), NULL); }
				callback(next->virtual_offset, next->get(), br);
			}
			// if (unsigned int const m = 1000 * (clock() - start) / CLOCKS_PER_SEC) { _ftprintf(stderr, _T("%u MiB/s for %u ms\n"), total_mft_size * 1000 / m / (1024 * 1024), m); }
		}
	}

	struct DirectorySizeCalculator
	{
		ProgressReporter *pProgressReporter;
		FileNameAttrs *pFileNameAttrs;
		FileNameAttrsDerived *pFileNameAttrsDerived;
		DataAttrs *pDataAttrs;
		DataAttrsDerived *pDataAttrsDerived;
		Children *pChildren;
		std::vector<unsigned short> seenCounts;
		size_t seenTotal, nFileRecords;

		DirectorySizeCalculator(
			ProgressReporter &progressReporter, Children &children,
			FileNameAttrs &fileNameAttrs, FileNameAttrsDerived &fileNameAttrsDerived, DataAttrs &dataAttrs,
			DataAttrsDerived &dataAttrsDerived, size_t const nFileRecords
		) :
			pProgressReporter(&progressReporter), pChildren(&children), pDataAttrs(&dataAttrs),
			pDataAttrsDerived(&dataAttrsDerived), pFileNameAttrs(&fileNameAttrs), pFileNameAttrsDerived(&fileNameAttrsDerived),
			seenTotal(0), nFileRecords(nFileRecords), seenCounts(nFileRecords)
		{ }

		std::pair<long long, long long> operator()(SegmentNumber const segmentNumber)
		{
			ProgressReporter &progressReporter = *pProgressReporter;
			progressReporter(nFileRecords * 2 + seenTotal, nFileRecords * 4);

			std::pair<long long, long long> result;
			if (segmentNumber >= pChildren->size())
			{ pChildren->resize(segmentNumber + 1); }
			for (boost::iterator_range<Children::value_type::const_iterator> r = (*pChildren)[segmentNumber]; !r.empty(); r.pop_front())
			{
				Children::value_type::value_type const &child = r.front();
				if (segmentNumber == child) { continue; }
				unsigned short const links = static_cast<unsigned short>(
					(child < pFileNameAttrsDerived->size() ? (*pFileNameAttrsDerived)[child].size() : 0) +
					boost::size(
						std::equal_range(
							pFileNameAttrs->begin(),
							pFileNameAttrs->end(),
							FileNameAttrs::value_type(child, FileNameAttribute()),
							first_less()
						)
					)
				);
				std::pair<long long, long long> const sizes = (*this)(child);
				unsigned short &seenCount = seenCounts[child];
				result.first += sizes.first * (seenCount + 1) / links - sizes.first * seenCount / links;
				result.second += sizes.second * (seenCount + 1) / links - sizes.second * seenCount / links;
				seenCount++;
			}
			std::pair<long long, long long> const childrenSizes = result;
			for (DataAttrs::iterator j =
				std::lower_bound(
					pDataAttrs->begin(),
					pDataAttrs->end(),
					DataAttrs::value_type(segmentNumber, DataAttrs::value_type::second_type()), first_less());
				j != pDataAttrs->end() && j->first == segmentNumber;
				++j)
			{
				DataAttrs::value_type::second_type &p = j->second;
				if (!p.first.second) { continue; }
				result.first += p.second.second.first;
				result.second += p.second.second.second;
				if (p.first.first == NTFS::AttributeIndexRoot || p.first.first == NTFS::AttributeIndexAllocation)
				{
					p.second.second.first += childrenSizes.first;
					p.second.second.second += childrenSizes.second;
				}
			}
			if (segmentNumber < pDataAttrsDerived->size())
			{
				for (boost::iterator_range<DataAttrsDerived::value_type::iterator> r = (*pDataAttrsDerived)[segmentNumber]; !r.empty(); r.pop_front())
				{
					DataAttrsDerived::value_type::value_type &p = r.front();
					if (!p.first.second) { continue; }
					result.first += p.second.second.first;
					result.second += p.second.second.second;
					if (p.first.first == NTFS::AttributeIndexRoot || p.first.first == NTFS::AttributeIndexAllocation)
					{
						p.second.second.first += childrenSizes.first;
						p.second.second.second += childrenSizes.second;
					}
				}
			}
			++seenTotal;
			return result;
		}
	};

	DirectorySizeCalculator(progressReporter, children, fileNameAttrs, fileNameAttrsDerived, dataAttrs, dataAttrsDerived, nFileRecords)(0x000000000005);

	FileNameAttrs::const_iterator const itFileNameAttrsEnd = fileNameAttrs.end();
	DataAttrs::const_iterator const itDataAttrsEnd = dataAttrs.end();
	FileNameAttrs::const_iterator itFileNameAttr = fileNameAttrs.begin();
	DataAttrs::const_iterator itDataAttr = dataAttrs.begin();

	for (boost::iterator_range<StandardInfoAttrs::const_iterator> range = standardInfoAttrs; !range.empty(); range.pop_front())
	{
		SegmentNumber const segmentNumber = range.front().first;

		progressReporter(nFileRecords * 3 + segmentNumber, nFileRecords * 4);

		while (itFileNameAttr != itFileNameAttrsEnd && itFileNameAttr->first < segmentNumber) { ++itFileNameAttr; }
		while (itDataAttr != itDataAttrsEnd && itDataAttr->first < segmentNumber) { ++itDataAttr; }

		StandardInformationAttribute const &standardInfoAttr = range.front().second;

		DataAttrs::const_iterator const itDataAttrBegin(itDataAttr);
		/*
		for (boost::iterator_range<FileNameAttrs::const_iterator> range =
			std::equal_range(
				fileNameAttrs.begin(),
				fileNameAttrs.end(),
				FileNameAttrs::value_type(segmentNumber, FileNameAttribute()),
				first_less()); !range.empty(); range.pop_front())
		*/
		while (itFileNameAttr != fileNameAttrs.end() && itFileNameAttr->first == segmentNumber)
		{
			itDataAttr = itDataAttrBegin;
			this->process_file_name(segmentNumber, standardInfoAttr, itFileNameAttr->second, itDataAttr, itDataAttrsEnd, dataAttrsDerived);
			++itFileNameAttr;
		}
		if (segmentNumber < fileNameAttrsDerived.size())
		{
			for (boost::iterator_range<FileNameAttrsDerived::value_type::const_iterator> range = fileNameAttrsDerived[segmentNumber]; !range.empty(); range.pop_front())
			{
				itDataAttr = itDataAttrBegin;
				this->process_file_name(segmentNumber, standardInfoAttr, range.front(), itDataAttr, itDataAttrsEnd, dataAttrsDerived);
			}
		}
	}
}

void NtfsIndex::process_file_name(
	SegmentNumber const segmentNumber,
	StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr,
	DataAttrs::const_iterator &itDataAttr, DataAttrs::const_iterator const itDataAttrsEnd, DataAttrsDerived const &dataAttrsDerived)
{
	/*
	for (boost::iterator_range<DataAttrs::const_iterator> range =
		std::equal_range(
			dataAttrs.begin(),
			dataAttrs.end(),
			DataAttrs::value_type(segmentNumber, DataAttrs::value_type::second_type()), first_less());
		!range.empty();
		range.pop_front())
	*/
	while (itDataAttr != itDataAttrsEnd && itDataAttr->first == segmentNumber)
	{
		this->process_file_data(segmentNumber, standardInfoAttr, fileNameAttr, itDataAttr->second); ++itDataAttr;
	}
	if (segmentNumber < dataAttrsDerived.size())
	{
		for (boost::iterator_range<DataAttrsDerived::value_type::const_iterator> range = dataAttrsDerived[segmentNumber]; !range.empty(); range.pop_front())
		{ this->process_file_data(segmentNumber, standardInfoAttr, fileNameAttr, range.front()); }
	}
}

void NtfsIndex::process_file_data(SegmentNumber const segmentNumber, StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr, DataAttribute const dataAttr)
{
	this->index.push_back(CombinedRecords::value_type(
		segmentNumber,
		CombinedRecords::value_type::second_type(
			standardInfoAttr,
			CombinedRecords::value_type::second_type::second_type(fileNameAttr, dataAttr))));
}

NtfsIndex::SegmentNumber NtfsIndex::get_name(SegmentNumber segmentNumber, std::basic_string<TCHAR> &s) const
{
	CombinedRecords::const_iterator const lb =
		std::lower_bound(
			this->index.begin(),
			this->index.end(),
			CombinedRecords::value_type(segmentNumber, CombinedRecord::second_type()), first_less());
	for (CombinedRecords::const_iterator i = lb; i != this->index.end() && i->first == segmentNumber; ++i)
	{
		if (i->second.second.second.second.first.second == 0)
		{
			return this->get_name_by_record(*i, s, false);
		}
	}
	throw std::domain_error("unable to resolve find name");
}

void NtfsIndex::swap(This &other)
{
	using std::swap;
	swap(this->names, other.names);
	swap(this->index, other.index);
}

std::pair<NtfsIndex::SubTString, std::pair<NtfsIndex::SubTString, NtfsIndex::SubTString> > NtfsIndex::get_name_by_record(CombinedRecord const &record) const
{
	TCHAR const *const data = this->names.data();
	NTFS::AttributeTypeCode const type = record.second.second.second.first.first;
	LPCTSTR const attribute_name = type != NTFS::AttributeData && type != NTFS::AttributeIndexRoot && type != NTFS::AttributeIndexAllocation ? NTFS::GetAttributeName(type) : _T("");
	return std::pair<SubTString, std::pair<SubTString, SubTString> >(
		SubTString(data + record.second.second.first.first.first,  data + record.second.second.first.first.first  + record.second.second.first.first.second),
		std::pair<SubTString, SubTString>(
			SubTString(data + record.second.second.second.second.first.first, data + record.second.second.second.second.first.first + record.second.second.second.second.first.second),
			SubTString(attribute_name, attribute_name + std::char_traits<TCHAR>::length(attribute_name))));
}

NtfsIndex::SegmentNumber NtfsIndex::get_name_by_record(CombinedRecord const &record, std::basic_string<TCHAR> &s, bool const include_attribute_name) const
{
	std::pair<SubTString, std::pair<SubTString, SubTString> > const p = this->get_name_by_record(record);
	s.append(p.first.begin(), p.first.end());
	if (!p.second.first.empty() || include_attribute_name && !p.second.second.empty())
	{
		s.append(1, _T(':'));
	}
	if (!p.second.first.empty())
	{
		s.append(p.second.first.begin(), p.second.first.end());
	}
	if (include_attribute_name && !p.second.second.empty())
	{
		s.append(1, _T(':'));
		s.append(p.second.second.begin(), p.second.second.end());
	}
	return record.second.second.first.second;
}

NtfsIndex::CombinedRecord const &NtfsIndex::at(size_t const i) const { return this->index.at(i); }
size_t NtfsIndex::size() const { return this->index.size(); }
