#include "stdafx.h"
#include "targetver.h"

#include "NtfsIndex.hpp"

#include <map>
#include <vector>

#include <boost/range/algorithm/equal_range.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/sub_range.hpp>

#include "CComCritSecLock.hpp"
#include "NTFS (1).h"
#include "NtfsReader.hpp"
#include "NtObjectMini.hpp"

template<class T1, class T2>
std::vector<std::pair<T1 const, T2> > &as_const(std::vector<std::pair<T1, T2> > &v) { return reinterpret_cast<std::vector<std::pair<T1 const, T2> > &>(v); }

class NtfsIndexImpl : public NtfsIndex
{
	typedef NtfsIndexImpl This;
	struct first_less { template<class P1, class P2> bool operator()(P1 const &a, P2 const &b) const { return a.first < b.first; } };

	typedef std::vector<std::pair<SegmentNumber, StandardInformationAttribute> > StandardInfoAttrs;
	typedef std::multimap<SegmentNumber, StandardInformationAttribute> StandardInfoAttrsDerived;
	typedef std::vector<std::pair<SegmentNumber, FileNameAttribute> > FileNameAttrs;
	typedef std::multimap<SegmentNumber, FileNameAttribute> FileNameAttrsDerived;
	typedef std::vector<std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > > DataAttrs;
	typedef std::multimap<DataAttrs::value_type::first_type, DataAttrs::value_type::second_type> DataAttrsDerived;

	typedef StandardInfoAttrs::const_iterator StandardInfoIt;
	typedef FileNameAttrs::const_iterator FileNameIt;
	typedef DataAttrs::const_iterator DataIt;

	std::basic_string<TCHAR> volumePath;
	std::basic_string<TCHAR> names1, names2;
	typedef std::vector<CombinedRecord> CombinedRecords;
	CombinedRecords index;
	winnt::NtEvent _event;

	void process_file_name(SegmentNumber const segmentNumber, StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr, DataAttrs const &dataAttrs, DataAttrsDerived const &dataAttrsDerived)
	{
		for (boost::iterator_range<DataAttrs::const_iterator> range = boost::equal_range(dataAttrs, std::make_pair(segmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute>()), first_less()); !range.empty(); range.pop_front())
		{ this->process_file_data(segmentNumber, standardInfoAttr, fileNameAttr, range.front().second.second); }
		for (boost::iterator_range<DataAttrsDerived::const_iterator> range = dataAttrsDerived.equal_range(segmentNumber); !range.empty(); range.pop_front())
		{ this->process_file_data(segmentNumber, standardInfoAttr, fileNameAttr, range.front().second.second); }
	}

	void process_file_data(SegmentNumber const segmentNumber, StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr, DataAttribute const dataAttr)
	{
		if (this->names2.size() + 2 * MAX_PATH > this->names2.capacity())
		{
			/* Old MSVCRT doesn't do this! */
			this->names2.reserve((this->names2.size() + fileNameAttr.first.second + 1 + dataAttr.first.second) * 2);
		}

		size_t const cchNames = this->names2.size();
		this->names2.append(&this->names1[fileNameAttr.first.first], fileNameAttr.first.second);
		if (dataAttr.first.second > 0)
		{
			this->names2.append(1, _T(':'));
			this->names2.append(&this->names1[dataAttr.first.first], dataAttr.first.second);
		}
		this->index.push_back(std::make_pair(
			segmentNumber,
			std::make_pair(
				standardInfoAttr,
				std::make_pair(
					std::make_pair(
						std::make_pair(
							static_cast<NameOffset>(cchNames),
							static_cast<NameLength>(this->names2.size() - cchNames)),
						fileNameAttr.second),
					dataAttr.second))));
	}

	NtfsIndexImpl(This const &);
	NtfsIndexImpl &operator =(This const &);
public:
	friend void swap(This &a, This &b) { a.swap(b); }

	void swap(This &other)
	{
		using std::swap;
		swap(this->names2, other.names2);
		swap(this->index, other.index);
	}

	CombinedRecord const &at(size_t const i) const { return this->index.at(i); }
	size_t size() const { return this->index.size(); }

	boost::iterator_range<TCHAR const *> get_name(FileNameAttribute::first_type const range) const
	{
		TCHAR const *const data = this->names2.data();
		return boost::make_iterator_range(&data[range.first], &data[range.first + range.second]);
	}

	SegmentNumber get_name(SegmentNumber segmentNumber, std::basic_string<TCHAR> &s) const
	{
		if (segmentNumber == 0x000000000005)
		{
			s.append(this->volumePath);
			return segmentNumber;
		}
		boost::sub_range<CombinedRecords const> const equal_range =
			boost::equal_range(this->index, std::make_pair(segmentNumber, CombinedRecord::second_type()), first_less());
		for (CombinedRecords::const_iterator i = equal_range.begin(); i != equal_range.end(); ++i)
		{
			FileNameAttribute const &fileNameAttr = i->second.second.first;
			s.append(this->names2.data() + fileNameAttr.first.first, fileNameAttr.first.second);
			return fileNameAttr.second;
		}
		throw std::domain_error("Could not find name for file record.");
	}

	NtfsIndexImpl(winnt::NtFile volume, winnt::NtEvent event, unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, bool volatile *pCached, bool volatile *pBackground)
		: volumePath(volume.GetPathOfVolume()), _event(event)
	{
		unsigned long const clusterSize = volume.GetClusterSize();
		std::auto_ptr<NtfsReader> const reader(NtfsReader::create(volume, pCached ? *pCached : true));

		bool prevBackground = pBackground ? !*pBackground : false;

		size_t const nFileRecords = reader->size();
		unsigned long const denom = (std::numeric_limits<unsigned long>::max)();

		StandardInfoAttrs standardInfoAttrs;
		FileNameAttrs fileNameAttrs;
		DataAttrs dataAttrs;

		StandardInfoAttrsDerived standardInfoAttrsDerived;
		FileNameAttrsDerived fileNameAttrsDerived;
		DataAttrsDerived dataAttrsDerived;

		typedef std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, std::basic_string<TCHAR> > > AttributeIdentifier;

		for (size_t segmentNumber = 0; segmentNumber < nFileRecords; segmentNumber = reader->find_next(segmentNumber))
		{
			if (pBackground != NULL && prevBackground != *pBackground)
			{
				prevBackground = *pBackground;
				SetThreadPriority(GetCurrentThread(), *pBackground ? 0x00010000 /*THREAD_MODE_BACKGROUND_BEGIN*/ : 0x00020000 /*THREAD_MODE_BACKGROUND_END*/);
			}
			this->_event.NtWaitForSingleObject();
			if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(segmentNumber) / (nFileRecords * 2))) == static_cast<long>(denom)) { return; }

			if (pCached) { reader->set_cached(*pCached); }
			NTFS::FILE_RECORD_SEGMENT_HEADER const *pRecord = static_cast<NTFS::FILE_RECORD_SEGMENT_HEADER const *>((*reader)[segmentNumber]);
			if (pRecord != NULL && (pRecord->Flags & 1) != 0)
			{
				NTFS::FILE_RECORD_SEGMENT_HEADER const &record = *pRecord;
				assert(record.GetSegmentNumber() == segmentNumber);
				bool const isBase = !record.BaseFileRecordSegment;
				SegmentNumber const baseSegment = static_cast<SegmentNumber>(isBase ? segmentNumber : record.BaseFileRecordSegment);
				for (NTFS::ATTRIBUTE_RECORD_HEADER const *ah = record.TryGetAttributeHeader(); ah != NULL; ah = ah->TryGetNextAttributeHeader())
				{{
					NTFS::ATTRIBUTE_RECORD_HEADER const &ah(*ah);
					long long sizeOnDisk = ah.IsNonResident ? 0 : ah.Length;
					if (ah.IsNonResident)
					{
						LONGLONG currentLcn = 0;
						LONGLONG nextVcn = ah.NonResident.LowestVCN;
						LONGLONG currentVcn = nextVcn;
						for (unsigned char const *pRun = ah.NonResident.GetMappingPairs(); *pRun != 0; pRun += NTFS::RunLength(pRun))
						{
							LONGLONG const runCount = NTFS::RunCount(pRun);
							nextVcn += runCount;
							currentLcn += NTFS::RunDeltaLCN(pRun);
							if (currentLcn != 0 ||
								ah.Type == NTFS::AttributeData && baseSegment == 0x000000000007 /* $Boot is a special case */)
							{ sizeOnDisk += runCount * clusterSize; }
							currentVcn = nextVcn;
						}
					}
					static TCHAR const I30[] = { _T('$'), _T('I'), _T('3'), _T('0') };
					bool const isI30 = ah.NameLength == sizeof(I30) / sizeof(*I30) && memcmp(I30, ah.GetName(), ah.NameLength) == 0;
					if (ah.Type == NTFS::AttributeStandardInformation)
					{
						NTFS::STANDARD_INFORMATION const &a = *static_cast<NTFS::STANDARD_INFORMATION const *>(ah.Resident.GetValue());
						StandardInfoAttrs::value_type const v = std::make_pair(
							baseSegment,
							std::make_pair(
								std::make_pair(
									winnt::NtSystem::RtlTimeToSecondsSince1980(a.CreationTime),
									winnt::NtSystem::RtlTimeToSecondsSince1980(a.LastModificationTime)),
								std::make_pair(
									winnt::NtSystem::RtlTimeToSecondsSince1980(a.LastAccessTime),
									a.FileAttributes)));
						if (isBase) { standardInfoAttrs.push_back(v); }
						else { standardInfoAttrsDerived.insert(v); }
					}
					if (ah.Type == NTFS::AttributeFileName)
					{
						NTFS::FILENAME_INFORMATION const &a = *static_cast<NTFS::FILENAME_INFORMATION const *>(ah.Resident.GetValue());
						if (a.Flags != 2)
						{
							if (this->names1.size() + a.FileNameLength > this->names1.capacity())
							{
								/* Old MSVCRT doesn't do this! */
								this->names1.reserve(this->names1.size() * 2);
							}
							FileNameAttrs::value_type const v = std::make_pair(
								baseSegment,
								std::make_pair(
									std::make_pair(
										static_cast<NameOffset>(this->names1.size()),
										static_cast<NameLength>(a.FileNameLength)),
									static_cast<SegmentNumber>(a.ParentDirectory & 0x0000FFFFFFFFFFFF)));
							if (isBase) { fileNameAttrs.push_back(v); }
							else { fileNameAttrsDerived.insert(v); }
							this->names1.append(a.FileName, a.FileNameLength);
						}
					}
					if (ah.Type == NTFS::AttributeData ||
						ah.Type == NTFS::AttributeIndexAllocation ||
						(
							ah.Type == NTFS::AttributeIndexRoot
							? (static_cast<NTFS::INDEX_ROOT const *>(ah.Resident.GetValue())->Header.Flags & 1) == 0
							: !isI30 && ah.NameLength > 0 && (ah.Type != NTFS::AttributeLoggedUtilityStream || ah.IsNonResident)
						))
					{
						size_t const cchNameOld = this->names1.size();
						if (!(ah.NameLength == 0 && ah.Type == NTFS::AttributeData || isI30 && (ah.Type == NTFS::AttributeIndexRoot || ah.Type == NTFS::AttributeIndexAllocation)))
						{
							if (this->names1.size() + MAX_PATH > this->names1.capacity())
							{
								/* Old MSVCRT doesn't do this! */
								this->names1.reserve((this->names1.size() + MAX_PATH) * 2);
							}
							this->names1.append(ah.GetName(), ah.NameLength);
							if (ah.Type != NTFS::AttributeData && ah.Type != NTFS::AttributeIndexRoot && ah.Type != NTFS::AttributeIndexAllocation)
							{
								this->names1.append(1, _T(':'));
								this->names1.append(NTFS::GetAttributeName(ah.Type));
							}
						}
						DataAttrs::value_type const v = std::make_pair(
							baseSegment,
							std::make_pair(
								ah.Type,
								std::make_pair(
									std::make_pair(
										static_cast<NameOffset>(cchNameOld),
										static_cast<NameLength>(this->names1.size() - cchNameOld)),
									std::make_pair(
										ah.IsNonResident ? ah.NonResident.DataSize : ah.Resident.ValueLength,
										sizeOnDisk))));
						
						bool found = false;
						if (ah.IsNonResident)
						{
							class DupeChecker
							{
								DupeChecker &operator =(DupeChecker const &) { throw std::logic_error(""); }
								std::basic_string<TCHAR> const &names1;
								NTFS::ATTRIBUTE_RECORD_HEADER const &ah;
								DataAttrs::value_type const &v;
							public:
								DupeChecker(std::basic_string<TCHAR> const &names1, NTFS::ATTRIBUTE_RECORD_HEADER const &ah, DataAttrs::value_type const &v)
									: names1(names1), ah(ah), v(v) { }
								bool operator()(DataAttrs::value_type::second_type &attr)
								{
									bool found = false;
									if (attr.first == ah.Type &&
										boost::equal(
											boost::make_iterator_range(ah.GetName(), ah.GetName() + ah.NameLength),
											boost::make_iterator_range(
												names1.data() + attr.second.first.first,
												names1.data() + attr.second.first.first + attr.second.first.second)))
									{
										if (ah.NonResident.LowestVCN == 0)
										{
											long long const oldSizeOnDisk = attr.second.second.second;
											attr = v.second;
											attr.second.second.second += oldSizeOnDisk;
										}
										else
										{
											attr.second.second.second += v.second.second.second.second;
										}
										found = true;
									}
									return found;
								}
							} dupeChecker(this->names1, ah, v);
							if (!found)
							{
								for (boost::iterator_range<DataAttrs::iterator> range = boost::equal_range(dataAttrs, std::make_pair(baseSegment, std::pair<NTFS::AttributeTypeCode, DataAttribute>()), first_less()); !range.empty(); range.pop_front())
								{
									found |= dupeChecker(range.front().second);
									if (found) { break; }
								}
							}
							if (!found)
							{
								for (boost::iterator_range<DataAttrsDerived::iterator> range = dataAttrsDerived.equal_range(baseSegment); !range.empty(); range.pop_front())
								{
									found |= dupeChecker(range.front().second);
									if (found) { break; }
								}
							}
						}
						if (!found)
						{
							if (isBase) { dataAttrs.push_back(v); }
							else { dataAttrsDerived.insert(v); }
						}
					}
				}}
			}
		}

		for (boost::iterator_range<StandardInfoAttrs::const_iterator> range = standardInfoAttrs; !range.empty(); range.pop_front())
		{
			SegmentNumber const segmentNumber = range.front().first;

			if (pBackground != NULL && prevBackground != *pBackground)
			{
				prevBackground = *pBackground;
				SetThreadPriority(GetCurrentThread(), *pBackground ? 0x00010000 /*THREAD_MODE_BACKGROUND_BEGIN*/ : 0x00020000 /*THREAD_MODE_BACKGROUND_END*/);
			}
			this->_event.NtWaitForSingleObject();
			if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(nFileRecords + segmentNumber) / (nFileRecords * 2))) == static_cast<long>(denom)) { return; }

			StandardInformationAttribute const &standardInfoAttr = range.front().second;

			for (boost::iterator_range<FileNameAttrs::const_iterator> range = boost::equal_range(fileNameAttrs, std::make_pair(segmentNumber, FileNameAttribute()), first_less()); !range.empty(); range.pop_front())
			{ this->process_file_name(segmentNumber, standardInfoAttr, range.front().second, dataAttrs, dataAttrsDerived); }
			for (boost::iterator_range<FileNameAttrsDerived::const_iterator> range = fileNameAttrsDerived.equal_range(segmentNumber); !range.empty(); range.pop_front())
			{ this->process_file_name(segmentNumber, standardInfoAttr, range.front().second, dataAttrs, dataAttrsDerived); }

		}
	}

	winnt::NtEvent event() const { return this->_event; }
};

NtfsIndex *NtfsIndex::create(winnt::NtFile const &volume, winnt::NtEvent const &event, unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, bool volatile *pCached, bool volatile *pBackground)
{
	return new NtfsIndexImpl(volume, event, pProgress, pCached, pBackground);
}