#include "stdafx.h"
#include "targetver.h"

#include "NtfsReader.hpp"

#include <map>
#include <memory>
#include <utility>
#include <valarray>
#include <vector>

#include <boost/ptr_container/ptr_map.hpp>

#include <atlbase.h>

#include "CComCritSecLock.hpp"
#include "NTFS (1).h"
#include "NtObjectMini.hpp"

class NtfsReaderImpl : public NtfsReader
{
	typedef NtfsReaderImpl This;
	winnt::NtFile volume;
	unsigned long diskNumber;
	std::vector<std::pair<unsigned long long, long long> > mftRetPtrs;
	unsigned long const clusterSize;
	unsigned long cbFileRecord;
	bool cached;
	bool background;
	size_t iFirstCachedOutputSegment;
	size_t iFirstCachedDirectSegment;
	std::valarray<unsigned char> outputBuffer;
	std::valarray<unsigned char> directBuffer;

	static class PhysicalDriveLocks : public ATL::CComAutoCriticalSection, private boost::ptr_map<unsigned long, ATL::CComAutoCriticalSection>
	{
	public:
		ATL::CComAutoCriticalSection &operator[](unsigned long const diskNumber)
		{
			{
				ATL::CComCritSecLock<ATL::CComAutoCriticalSection> const lock(*this);
				if (this->find(diskNumber) == this->end())
				{ this->insert(diskNumber, std::auto_ptr<ATL::CComAutoCriticalSection>(new ATL::CComAutoCriticalSection())); }
			}
			return this->at(diskNumber);
		}
	} physicalDriveLocks;

	NtfsReaderImpl(This const &);
	NtfsReaderImpl &operator =(This const &);
public:
	NtfsReaderImpl(winnt::NtFile const &volume) :
		volume(volume),
		diskNumber(static_cast<unsigned long>(-1)),
		clusterSize(volume.GetClusterSize()),
		cbFileRecord(volume.FsctlGetVolumeData().BytesPerFileRecordSegment),
		cached(false),
		background(true),
		iFirstCachedOutputSegment(static_cast<size_t>(-1)),
		iFirstCachedDirectSegment(static_cast<size_t>(-1)),
		outputBuffer(offsetof(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer) + cbFileRecord),
		directBuffer(256 * cbFileRecord)
	{
		try
		{
			std::vector<DISK_EXTENT> const extents = volume.IoctlVolumeGetVolumeDiskExtents();
			if (extents.size() == 1) { this->diskNumber = extents.back().DiskNumber; }
		}
		catch (CStructured_Exception &) { }

		winnt::NtFile mft = winnt::NtFile::NtOpenFile(winnt::ObjectAttributes(0x0001000000000000, volume.get()), winnt::Access::QueryAttributes | winnt::Access::Synchronize, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_BY_FILE_ID);
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
		for (size_t i = 0; i < retPtrsBuf.ExtentCount; i++)
		{ this->mftRetPtrs.push_back(std::make_pair(retPtrsBuf.Extents[i].NextVcn.QuadPart, retPtrsBuf.Extents[i].Lcn.QuadPart)); }


		// Tune the block size
		if (false)
		{
			size_t const timeoutMillis = 200;
			std::pair<long long, long long> const benchmarkStart; //= QueryPerformanceCounter()
			std::multimap<size_t, long double> perfs;
			unsigned int const subdivisions = 1;
			size_t const maxSize = 1 << 22;
			bool stop = false;
			for (size_t k = 0; !stop && k < 5; k++)
			{
				for (unsigned long blockSize = 16 * this->cbFileRecord; !stop && blockSize < maxSize; blockSize *= 2)
				{
					for (unsigned int l = 0; !stop && l < subdivisions; l++)
					{
						unsigned long const diff = l * blockSize / subdivisions;
						if (diff % this->cbFileRecord == 0)
						{
							this->directBuffer.resize((std::max)(blockSize + diff, clusterSize));
							std::pair<long long, long long> const start; //= QueryPerformanceCounter()
							size_t const cbRead = volume.NtReadFileVirtual(
								std::make_pair(0ULL, std::make_pair(this->mftRetPtrs.size(), &this->mftRetPtrs[0])),
								this->clusterSize, 0, &this->directBuffer[0], static_cast<unsigned long>(this->directBuffer.size() / cbFileRecord * cbFileRecord));
							std::pair<long long, long long> const end; //= QueryPerformanceCounter()
							perfs.insert(std::make_pair(this->directBuffer.size(), cbRead * static_cast<long double>(end.second) / (end.first - start.first))); 
							if (!perfs.empty())
							{
								if (1000 * (end.first - benchmarkStart.first) / benchmarkStart.second > timeoutMillis)
								{
									stop = true;
								}
							}
						}
					}
				}
			}
			std::map<size_t, long double> avgPerfs;
			for (std::multimap<size_t, long double>::iterator i = perfs.begin(); i != perfs.end(); ++i)
			{
				size_t const k = static_cast<size_t>(std::distance(perfs.lower_bound(i->first), i));
				avgPerfs[i->first] = (avgPerfs[i->first] * k + i->second) / (k + 1);
			}
			std::pair<size_t, long double> bestPerf;
			for (std::map<size_t, long double>::const_iterator i = avgPerfs.begin(); i != avgPerfs.end(); ++i)
			{
				if (i->second > bestPerf.second) { bestPerf = *i; }
				//std::cout << (i->first / (1 << 10)) << " KiB: " << static_cast<size_t>(i->second / (1 << 20)) << " MiB/s" << std::endl;
			}
			//std::cout << (bestPerf.first / (1 << 10)) << " KiB: " << static_cast<size_t>(bestPerf.second / (1 << 20)) << " MiB/s" << std::endl;
		}
	}

	size_t size() const { return static_cast<size_t>(static_cast<unsigned long long>(mftRetPtrs.back().first) * clusterSize / this->cbFileRecord); }

	bool set_background(bool value) { using std::swap; swap(value, this->background); return value; }

	size_t find_next(size_t const i)  // might return a value > this->size()!!!
	{
		size_t j = i;
		size_t d = 1;
		if (this->cached && (*this)[j + d] == NULL)
		{
			while (0 < d && j + d < this->size())
			{
				size_t k = j + d;
				if (this->get(k, true) != NULL && j < k)
				{
					if (d > 1)
					{
						j += d / 2;
						d = 1;
					}
					else { break; }
				}
				else { d *= 2; }
			}
		}
		j += d;
		d = 0;
		using std::min;
		j = min(j, this->size());
		return j;
	}

	struct ScopedIoPriority
	{
		winnt::NtFile *pVolume;
		IO_PRIORITY_HINT old;
		ScopedIoPriority(winnt::NtFile &volume, bool const lower)
			: pVolume(&volume), old(volume.GetIoPriorityHint())
		{ pVolume->SetIoPriorityHint(lower ? IoPriorityVeryLow : old); }
		~ScopedIoPriority() { pVolume->SetIoPriorityHint(old); }
	};

	NTFS::FILE_RECORD_SEGMENT_HEADER const *get(size_t &i /* might be lower */, bool cached)
	{
		NTFS::FILE_RECORD_SEGMENT_HEADER const *result;
		try
		{
			if (cached)
			{
				try
				{
					NTFS_FILE_RECORD_OUTPUT_BUFFER &output = reinterpret_cast<NTFS_FILE_RECORD_OUTPUT_BUFFER &>(this->outputBuffer[0]);
					if (this->iFirstCachedOutputSegment != i)
					{
						ATL::CComCritSecLock<ATL::CComAutoCriticalSection> const driveLock(physicalDriveLocks[this->diskNumber]);
						ScopedIoPriority const setPriority(volume, this->background);
						volume.FsctlGetNtfsFileRecordFloor(i, &output, this->outputBuffer.size());
					}
					this->iFirstCachedOutputSegment = i;
					i = static_cast<size_t>(output.FileReferenceNumber.QuadPart & 0x0000FFFFFFFFFFFF);
					NTFS::FILE_RECORD_SEGMENT_HEADER &record = reinterpret_cast<NTFS::FILE_RECORD_SEGMENT_HEADER &>(output.FileRecordBuffer);

					record.SegmentNumberLower = static_cast<unsigned long>(i);
					record.SegmentNumberUpper_or_USA_or_UnknownReserved = static_cast<unsigned short>(static_cast<unsigned long long>(i) >> (sizeof(record.SegmentNumberLower) * CHAR_BIT));

					result = &record;
				}
				catch (CStructured_Exception &ex)
				{
					if (ex.GetSENumber() != STATUS_INVALID_PARAMETER) { throw; }
					result = NULL;
				}
			}
			else
			{
				if (i < this->iFirstCachedDirectSegment || this->iFirstCachedDirectSegment + this->directBuffer.size() / this->cbFileRecord <= i)
				{
					size_t const nCached = this->directBuffer.size() / this->cbFileRecord;
					size_t const nReadBehind = i < this->iFirstCachedDirectSegment ? nCached - 1 : 0;
					using std::max;
					this->iFirstCachedDirectSegment = max(i, nReadBehind) - nReadBehind;
					size_t cbRead;
					{
						ATL::CComCritSecLock<ATL::CComAutoCriticalSection> const driveLock(physicalDriveLocks[this->diskNumber]);
						ScopedIoPriority const setPriority(volume, this->background);
						cbRead = volume.NtReadFileVirtual(
							std::make_pair(0ULL, std::make_pair(this->mftRetPtrs.size(), &this->mftRetPtrs[0])),
							this->clusterSize, this->iFirstCachedDirectSegment * this->cbFileRecord, &this->directBuffer[0], static_cast<unsigned long>(this->directBuffer.size() / cbFileRecord * cbFileRecord));
					}
					for (size_t j = 0; j < cbRead / this->cbFileRecord; j++)
					{
						NTFS::FILE_RECORD_SEGMENT_HEADER &record =
							reinterpret_cast<NTFS::FILE_RECORD_SEGMENT_HEADER &>(this->directBuffer[j * this->cbFileRecord]);
						if (!record.MultiSectorHeader.TryUnFixup(record.BytesInUse))
						{
							if ((record.Flags & 1) == 0)
							{ winnt::NtStatus(STATUS_FILE_CORRUPT_ERROR).CheckAndThrow(); }
						}
						record.SegmentNumberLower = static_cast<unsigned long>(this->iFirstCachedDirectSegment + j);
						record.SegmentNumberUpper_or_USA_or_UnknownReserved = static_cast<unsigned short>(static_cast<unsigned long long>(this->iFirstCachedDirectSegment + j) >> (sizeof(record.SegmentNumberLower) * CHAR_BIT));
					}
				}
				{
					NTFS::FILE_RECORD_SEGMENT_HEADER const &record =
						reinterpret_cast<NTFS::FILE_RECORD_SEGMENT_HEADER const &>(this->directBuffer[(i - this->iFirstCachedDirectSegment) * this->cbFileRecord]);
					result = (record.Flags & 1) != 0 ? &record : NULL;
				}
			}
		}
		catch (CStructured_Exception &ex)
		{
			if (ex.GetSENumber() != STATUS_IO_DEVICE_ERROR)
			{
				throw;
			}
			result = NULL;
		}
		return result;
	}

	void const *operator[](size_t const i)
	{
		size_t j = i;
		NTFS::FILE_RECORD_SEGMENT_HEADER const *const result = this->get(j, this->cached);
		return i == j ? result : NULL;
	}

};
NtfsReaderImpl::PhysicalDriveLocks NtfsReaderImpl::physicalDriveLocks;

NtfsReader *NtfsReader::create(winnt::NtFile const &volume)
{
	return new NtfsReaderImpl(volume);
}