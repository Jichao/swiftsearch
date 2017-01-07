#pragma once

struct CThemedListViewCtrl : public WTL::CListViewCtrl,
	public WTL::CThemeImpl<CThemedListViewCtrl> {
	using WTL::CListViewCtrl::Attach;
};
struct CacheInfo {
	CacheInfo() : valid(false), iIconSmall(-1), iIconLarge(-1), iIconExtraLarge(-1)
	{
		this->szTypeName[0] = _T('\0');
	}
	bool valid;
	int iIconSmall, iIconLarge, iIconExtraLarge;
	TCHAR szTypeName[80];
	std::tstring description;
};

class Result {
	typedef NtfsIndex::key_type second_type;
	typedef size_t third_type;
public:
	typedef boost::intrusive_ptr<NtfsIndex volatile const> first_type;
	struct depth_compare {
		bool operator()(Result const &a, Result const &b) const
		{
			return a.depth < b.depth;
		}
	};
	first_type index;
	second_type key;
	third_type depth;
	Result() : index(), key(), depth() { }
	Result(first_type const &first, second_type const &second,
		third_type const &third) : index(first), key(second), depth(third) { }
};
typedef std::vector<Result> Results;

template<class StrCmp>
class NameComparator {
	StrCmp less;
public:
	NameComparator(StrCmp const &less) : less(less) { }
	bool operator()(Results::value_type const &a, Results::value_type const &b)
	{
		bool less = this->less(a.file_name(), b.file_name());
		if (!less && !this->less(b.file_name(), a.file_name())) {
			less = this->less(a.stream_name(), b.stream_name());
		}
		return less;
	}
	bool operator()(Results::value_type const &a, Results::value_type const &b) const
	{
		bool less = this->less(a.file_name(), b.file_name());
		if (!less && !this->less(b.file_name(), a.file_name())) {
			less = this->less(a.stream_name(), b.stream_name());
		}
		return less;
	}
};

template<class StrCmp>
static NameComparator<StrCmp> name_comparator(StrCmp const &cmp)
{
	return NameComparator<StrCmp>(cmp);
}

