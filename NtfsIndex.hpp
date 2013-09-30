#pragma once

#include <stddef.h>
#include <tchar.h>

#include <utility>

#include <boost/range/iterator_range.hpp>

namespace NTFS { enum AttributeTypeCode; }
namespace winnt { class NtFile; class NtObject; }

class NtfsIndex
{
	typedef NtfsIndex This;
	typedef boost::iterator_range<TCHAR const *> SubTString;

public:
	static unsigned long const PROGRESS_CANCEL_REQUESTED = ULONG_MAX;  // Do NOT change the type of this variable! MUST be 'unsigned long'!
	typedef unsigned long NameOffset;
	typedef unsigned int NameLength;
	typedef unsigned long SegmentNumber;
	typedef std::pair<std::pair<long long, long long>, std::pair<long long, unsigned long> > StandardInformationAttribute;
	typedef std::pair<std::pair<NameOffset /*file name*/, NameLength>, SegmentNumber /* parent */> FileNameAttribute;
	typedef std::pair<
		std::pair<NTFS::AttributeTypeCode, bool /* file valid? (not deleted) */>,
		std::pair<std::pair<NameOffset /*stream name*/, NameLength>, std::pair<long long /*logical size*/, long long /*size on disk*/> >
	> DataAttribute;
	typedef std::pair<
		SegmentNumber,
		std::pair<
			StandardInformationAttribute,
			std::pair<FileNameAttribute, DataAttribute>
		>
	> CombinedRecord;

	friend void swap(This &a, This &b) { a.swap(b); }
	void swap(This &other);

	CombinedRecord const &at(size_t const i) const;
	size_t size() const;

	SegmentNumber get_name_by_record(CombinedRecord const &record, std::basic_string<TCHAR> &s, bool const include_attribute_name) const;
	std::pair<SubTString, std::pair<SubTString, SubTString> > get_name_by_record(CombinedRecord const &record) const;

	std::pair<SubTString, std::pair<SubTString, SubTString> > get_name_by_index(size_t const i) const
	{
		return this->get_name_by_record(this->at(i));
	}

	SegmentNumber get_name(SegmentNumber segmentNumber, std::basic_string<TCHAR> &s) const;

	typedef std::vector<std::vector<SegmentNumber> > Children;

	NtfsIndex(
		winnt::NtFile &volume, std::basic_string<TCHAR> const &win32Path, winnt::NtObject &is_allowed_event,
		unsigned long volatile *const pProgress  /* out of numeric_limits::max() */, unsigned long volatile *const pSpeed, bool volatile *pBackground);

	std::basic_string<TCHAR> const &drive() const { return this->_win32Path; }

private:
	NtfsIndex(This const &);
	NtfsIndex &operator =(This const &);

	typedef std::vector<std::pair<SegmentNumber, StandardInformationAttribute> > StandardInfoAttrs;
	typedef std::vector<std::pair<SegmentNumber, FileNameAttribute> > FileNameAttrs;
	typedef std::vector<std::vector<FileNameAttribute> > FileNameAttrsDerived;
	typedef std::vector<std::pair<SegmentNumber, DataAttribute> > DataAttrs;
	typedef std::vector<std::vector<DataAttrs::value_type::second_type> > DataAttrsDerived;

	typedef StandardInfoAttrs::const_iterator StandardInfoIt;
	typedef FileNameAttrs::const_iterator FileNameIt;
	typedef std::vector<CombinedRecord> CombinedRecords;

	void process_file_name(
		SegmentNumber const segmentNumber,
		StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr,
		DataAttrs::const_iterator &itDataAttr, DataAttrs::const_iterator const itDataAttrsEnd, DataAttrsDerived const &dataAttrsDerived);

	void process_file_data(SegmentNumber const segmentNumber, StandardInformationAttribute const standardInfoAttr, FileNameAttribute const fileNameAttr, DataAttribute const dataAttr);

	std::basic_string<TCHAR> _win32Path;
	std::basic_string<TCHAR> names;
	CombinedRecords index;
};
