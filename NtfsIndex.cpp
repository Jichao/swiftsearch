#include "stdafx.h"
#include "targetver.h"

#include "NtfsIndex.hpp"

#include <map>

#include <boost/range/algorithm/equal_range.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "CComCritSecLock.hpp"
#include "NTFS (1).h"
#include "NtfsReader.hpp"
#include "NtObjectMini.hpp"

class NtfsIndexImpl : public NtfsIndex
{
	typedef NtfsIndexImpl This;
	struct first_less { template<class Pair> bool operator()(Pair const &a, Pair const &b) const { return a.first < b.first; } };

	std::basic_string<TCHAR> volumePath;
	std::basic_string<TCHAR> names;
	std::vector<CombinedRecord> index;
	winnt::NtEvent _event;

	NtfsIndexImpl(This const &);
	NtfsIndexImpl &operator =(This const &);
public:
	friend void swap(This &a, This &b) { a.swap(b); }

	void swap(This &other)
	{
		using std::swap;
		swap(this->names, other.names);
		swap(this->index, other.index);
	}

	CombinedRecord const &at(size_t const i) const { return this->index.at(i); }
	size_t size() const { return this->index.size(); }

	boost::iterator_range<TCHAR const *> get_name(FileNameAttribute::first_type const range) const
	{
		TCHAR const *const data = this->names.data();
		return boost::make_iterator_range(&data[range.first], &data[range.first + range.second]);
	}

	SegmentNumber get_name(SegmentNumber segmentNumber, std::basic_string<TCHAR> &s) const
	{
		if (segmentNumber == 0x000000000005)
		{
			s.append(this->volumePath);
			return segmentNumber;
		}
		typedef std::vector<CombinedRecord> CombinedRecords;
		boost::sub_range<CombinedRecords const> const equal_range =
			boost::equal_range(this->index, make_pair(segmentNumber, CombinedRecord::second_type()), first_less());
		for (CombinedRecords::const_iterator i = equal_range.begin(); i != equal_range.end(); ++i)
		{
			FileNameAttribute const &fileNameAttr = i->second.second.first;
			s.append(this->names.data() + fileNameAttr.first.first, fileNameAttr.first.second);
			return fileNameAttr.second;
		}
		throw std::domain_error("Could not find name for file record.");
	}

	NtfsIndexImpl(winnt::NtFile volume, winnt::NtEvent event, unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, bool volatile *pCached, bool volatile *pBackground)
		: volumePath(volume.GetPathOfVolume()), _event(event)
	{
		unsigned long const clusterSize = volume.GetClusterSize();
		using std::make_pair;
		boost::shared_ptr<NtfsReader> reader(NtfsReader::create(volume, pCached ? *pCached : true));

		bool prevBackground = pBackground ? !*pBackground : false;
		size_t counter = 0;
		std::basic_string<TCHAR> localNames;

		std::vector<std::pair<SegmentNumber, StandardInformationAttribute> > standardInfoAttrs;
		std::vector<std::pair<SegmentNumber, FileNameAttribute> > fileNameAttrs;
		std::vector<std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > > dataAttrs;
		{
			std::vector<std::pair<SegmentNumber, StandardInformationAttribute> > standardInfoAttrsDerived;
			std::vector<std::pair<SegmentNumber, FileNameAttribute> > fileNameAttrsDerived;
			std::vector<std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > > dataAttrsDerived;

			typedef std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, std::basic_string<TCHAR> > > AttributeIdentifier;
			std::map<AttributeIdentifier, long long> uncountedBytesInHigherVcns;
			unsigned long const denom = (std::numeric_limits<unsigned long>::max)();

			if (pBackground != NULL && prevBackground != *pBackground)
			{
				prevBackground = *pBackground;
				SetThreadPriority(GetCurrentThread(), *pBackground ? 0x00010000 /*THREAD_MODE_BACKGROUND_BEGIN*/ : 0x00020000 /*THREAD_MODE_BACKGROUND_END*/);
			}

			size_t const nFileRecords = reader->size();
			for (size_t i = 0; i < nFileRecords; i = reader->find_next(i))
			{
				if (counter++ % 256 == 0) { this->_event.NtWaitForSingleObject(); }

				if (pCached) { reader->set_cached(*pCached); }
				NTFS::FILE_RECORD_SEGMENT_HEADER const *pRecord = static_cast<NTFS::FILE_RECORD_SEGMENT_HEADER const *>((*reader)[i]);
				if (pRecord != NULL && (pRecord->Flags & 1) != 0)
				{
					NTFS::FILE_RECORD_SEGMENT_HEADER const &record = *pRecord;
					bool const isBase = !record.BaseFileRecordSegment;
					SegmentNumber const baseSegment = static_cast<SegmentNumber>(isBase ? record.GetSegmentNumber() : record.BaseFileRecordSegment);
					std::pair<size_t, size_t> const progress(static_cast<size_t>(record.GetSegmentNumber()), nFileRecords + 4);
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
						if (ah.IsNonResident && ah.NonResident.LowestVCN != 0)
						{
							uncountedBytesInHigherVcns[std::make_pair(baseSegment, std::make_pair(ah.Type, std::basic_string<TCHAR>(ah.GetName(), ah.NameLength)))] += sizeOnDisk;
						}
						else
						{
							static TCHAR const I30[] = { _T('$'), _T('I'), _T('3'), _T('0') };
							bool const isI30 = ah.NameLength == sizeof(I30) / sizeof(*I30) && memcmp(I30, ah.GetName(), ah.NameLength) == 0;
							if (ah.Type == NTFS::AttributeStandardInformation)
							{
								NTFS::STANDARD_INFORMATION const &a = *static_cast<NTFS::STANDARD_INFORMATION const *>(ah.Resident.GetValue());
								(isBase ? standardInfoAttrs : standardInfoAttrsDerived).push_back(make_pair(
									baseSegment,
									make_pair(
										make_pair(
											winnt::NtSystem::RtlTimeToSecondsSince1980(a.CreationTime),
											winnt::NtSystem::RtlTimeToSecondsSince1980(a.LastModificationTime)),
										make_pair(
											winnt::NtSystem::RtlTimeToSecondsSince1980(a.LastAccessTime),
											a.FileAttributes))));
							}
							if (ah.Type == NTFS::AttributeFileName)
							{
								NTFS::FILENAME_INFORMATION const &a = *static_cast<NTFS::FILENAME_INFORMATION const *>(ah.Resident.GetValue());
								if (a.Flags != 2)
								{
									if (localNames.size() + a.FileNameLength > localNames.capacity())
									{
										/* Old MSVCRT doesn't do this! */
										localNames.reserve(localNames.size() * 2);
									}
									(isBase ? fileNameAttrs : fileNameAttrsDerived).push_back(make_pair(
										baseSegment,
										make_pair(
											make_pair(
												static_cast<NameOffset>(localNames.size()),
												static_cast<NameLength>(a.FileNameLength)),
											static_cast<SegmentNumber>(a.ParentDirectory & 0x0000FFFFFFFFFFFF))));
									localNames.append(a.FileName, a.FileNameLength);
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
								size_t const cchNameOld = localNames.size();
								if (!(ah.NameLength == 0 && ah.Type == NTFS::AttributeData || isI30 && (ah.Type == NTFS::AttributeIndexRoot || ah.Type == NTFS::AttributeIndexAllocation)))
								{
									if (localNames.size() + MAX_PATH > localNames.capacity())
									{
										/* Old MSVCRT doesn't do this! */
										localNames.reserve((localNames.size() + MAX_PATH) * 2);
									}
									localNames.append(ah.GetName(), ah.NameLength);
									if (ah.Type != NTFS::AttributeData && ah.Type != NTFS::AttributeIndexRoot && ah.Type != NTFS::AttributeIndexAllocation)
									{
										localNames.append(1, _T(':'));
										localNames.append(NTFS::GetAttributeName(ah.Type));
									}
								}
								(isBase ? dataAttrs : dataAttrsDerived).push_back(make_pair(
									baseSegment,
									make_pair(
										ah.Type,
										make_pair(
											make_pair(
												static_cast<NameOffset>(cchNameOld),
												static_cast<NameLength>(localNames.size() - cchNameOld)),
											make_pair(
												ah.IsNonResident ? ah.NonResident.DataSize : ah.Resident.ValueLength,
												sizeOnDisk)))));
							}
						}
					}}

					if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(progress.first) / progress.second)) == static_cast<long>(denom))
					{
						return;
					}
				}
			}

			std::pair<size_t, size_t> const progress(nFileRecords, nFileRecords + 3);

			std::stable_sort(standardInfoAttrsDerived.begin(), standardInfoAttrsDerived.end(), first_less());
			if (size_t const n = standardInfoAttrs.size())
			{
				standardInfoAttrs.insert(standardInfoAttrs.end(), standardInfoAttrsDerived.begin(), standardInfoAttrsDerived.end());
				std::inplace_merge(standardInfoAttrs.begin(), standardInfoAttrs.begin() + static_cast<ptrdiff_t>(n), standardInfoAttrs.end(), first_less());
				if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(progress.first + 1) / progress.second)) == static_cast<long>(denom))
				{
					return;
				}
			}

			std::stable_sort(fileNameAttrsDerived.begin(), fileNameAttrsDerived.end(), first_less());
			if (size_t const n = fileNameAttrs.size())
			{
				fileNameAttrs.insert(fileNameAttrs.end(), fileNameAttrsDerived.begin(), fileNameAttrsDerived.end());
				std::inplace_merge(fileNameAttrs.begin(), fileNameAttrs.begin() + static_cast<ptrdiff_t>(n), fileNameAttrs.end(), first_less());
				if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(progress.first + 2) / progress.second)) == static_cast<long>(denom))
				{
					return;
				}
			}

			std::stable_sort(dataAttrsDerived.begin(), dataAttrsDerived.end(), first_less());
			if (size_t const n = dataAttrs.size())
			{
				dataAttrs.insert(dataAttrs.end(), dataAttrsDerived.begin(), dataAttrsDerived.end());
				std::inplace_merge(dataAttrs.begin(), dataAttrs.begin() + static_cast<ptrdiff_t>(n), dataAttrs.end(), first_less());
				if (pProgress && InterlockedExchange(reinterpret_cast<long volatile *>(pProgress), static_cast<unsigned long>(denom * static_cast<unsigned long long>(progress.first + 3) / progress.second)) == static_cast<long>(denom))
				{
					return;
				}
			}

			for (std::map<AttributeIdentifier, long long>::const_iterator i = uncountedBytesInHigherVcns.begin(); i != uncountedBytesInHigherVcns.end(); ++i)
			{
				for (boost::sub_range<std::vector<std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > > > range =
					boost::equal_range(dataAttrs, std::make_pair(i->first.first, std::pair<NTFS::AttributeTypeCode, DataAttribute>()), first_less());
					!range.empty();
					range.pop_front())
				{
					std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > &attr = range.front();
					if (i->first.second.first == attr.second.first &&
						boost::equal(
							i->first.second.second,
							boost::make_iterator_range(
								localNames.data() + attr.second.second.first.first,
								localNames.data() + attr.second.second.first.first + attr.second.second.first.second)))
					{
						attr.second.second.second.second += i->second;
					}
				}
			}
		}
		{
			TCHAR const *const localNames(localNames.data());

			typedef std::vector<std::pair<SegmentNumber, StandardInformationAttribute> >::const_iterator StandardInfoIt;
			typedef std::vector<std::pair<SegmentNumber, FileNameAttribute> >::const_iterator FileNameIt;
			typedef std::vector<std::pair<SegmentNumber, std::pair<NTFS::AttributeTypeCode, DataAttribute> > >::const_iterator DataIt;
			FileNameIt itFileNameAttr = fileNameAttrs.begin();
			FileNameIt const itFileNameAttrEnd = fileNameAttrs.end();
			DataIt itDataAttr = dataAttrs.begin();
			DataIt const itDataAttrEnd = dataAttrs.end();
			for (StandardInfoIt s = standardInfoAttrs.begin(); s != standardInfoAttrs.end(); ++s)
			{
				if (counter++ % 256 == 0) { this->_event.NtWaitForSingleObject(); }

				SegmentNumber const segmentNumber = s->first;
				StandardInformationAttribute const &standardInfoAttr = s->second;

				while (itFileNameAttr != itFileNameAttrEnd && itFileNameAttr->first < segmentNumber) { ++itFileNameAttr; }
				while (itDataAttr != itDataAttrEnd && itDataAttr->first < segmentNumber) { ++itDataAttr; }

				for (FileNameIt n = itFileNameAttr; n != itFileNameAttrEnd && n->first == segmentNumber; ++n)
				{
					FileNameAttribute const &fileNameAttr = n->second;
					for (DataIt d = itDataAttr; d != itDataAttrEnd && d->first == segmentNumber; ++d)
					{
						DataAttribute const &dataAttr = d->second.second;
						if (this->names.size() + 2 * MAX_PATH > this->names.capacity())
						{
							/* Old MSVCRT doesn't do this! */
							this->names.reserve((this->names.size() + fileNameAttr.first.second + 1 + dataAttr.first.second) * 2);
						}

						size_t const cchNames = this->names.size();
						this->names.append(&localNames[fileNameAttr.first.first], fileNameAttr.first.second);
						if (dataAttr.first.second > 0)
						{
							this->names.append(1, _T(':'));
							this->names.append(&localNames[dataAttr.first.first], dataAttr.first.second);
						}
						this->index.push_back(make_pair(
							segmentNumber,
							make_pair(
								standardInfoAttr,
								make_pair(
									make_pair(
										make_pair(
											static_cast<NameOffset>(cchNames),
											static_cast<NameLength>(this->names.size() - cchNames)),
										fileNameAttr.second),
									dataAttr.second))));
					}
				}
			}
		}
	}

	winnt::NtEvent event() const { return this->_event; }
};

NtfsIndex *NtfsIndex::create(winnt::NtFile const &volume, winnt::NtEvent const &event, unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, bool volatile *pCached, bool volatile *pBackground)
{
	return new NtfsIndexImpl(volume, event, pProgress, pCached, pBackground);
}