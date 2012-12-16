#pragma once

#include <stddef.h>
#include <tchar.h>

#include <utility>

#include <boost/range/iterator_range.hpp>

namespace winnt { class NtFile; class NtEvent; }

class __declspec(novtable) NtfsIndex
{
public:
	typedef unsigned long NameOffset;
	typedef unsigned int NameLength;
	typedef unsigned long SegmentNumber;
	typedef std::pair<std::pair<unsigned long, unsigned long>, std::pair<unsigned long, unsigned long> > StandardInformationAttribute;
	typedef std::pair<std::pair<NameOffset /*file name*/, NameLength>, SegmentNumber /* parent */> FileNameAttribute;
	typedef std::pair<std::pair<NameOffset /*stream name*/, NameLength>, std::pair<long long /*logical size*/, long long /*size on disk*/> > DataAttribute;
	typedef std::pair<SegmentNumber, std::pair<StandardInformationAttribute, std::pair<std::pair<std::pair<NameOffset /*file name*/, NameOffset /*stream name*/>, std::pair<NameLength, NameLength> >, std::pair<FileNameAttribute::second_type, DataAttribute::second_type> > > > CombinedRecord;

	virtual ~NtfsIndex() { }
	virtual size_t size() const = 0;
	virtual CombinedRecord const &at(size_t const i) const = 0;
	virtual SegmentNumber get_name(SegmentNumber segmentNumber, std::basic_string<TCHAR> &s) const = 0;
	virtual void get_name_by_index(size_t const i, std::basic_string<TCHAR> &s) const = 0;
	virtual std::basic_string<TCHAR> const &volumePath() const = 0;

	static NtfsIndex *create(winnt::NtFile const &volume, winnt::NtEvent const &event, unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, bool volatile *pCached, bool volatile *pBackground);
};
