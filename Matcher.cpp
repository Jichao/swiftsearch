#include "stdafx.h"
#include "targetver.h"

#include "Matcher.hpp"

#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/join.hpp>

#define clear() resize(0)
#include <boost/exception/info.hpp>
#undef clear
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#define clear() resize(0)
#define push_back(x) operator +=(x)
#include <boost/xpressive/detail/dynamic/parser_traits.hpp>
#undef  push_back
#undef clear
#include <boost/xpressive/match_results.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>

struct tchar_ci_traits : public std::char_traits<TCHAR>
{
	typedef std::char_traits<TCHAR> Base;
	static bool eq(TCHAR c1, TCHAR c2)
	{
		return c1 < SCHAR_MAX
			? c1 == c2 ||
				_T('A') <= c1 && c1 <= _T('Z') && c1 - c2 == _T('A') - _T('a') ||
				_T('A') <= c2 && c2 <= _T('Z') && c2 - c1 == _T('A') - _T('a')
			: _totupper(c1) == _totupper(c2);
	}
	static bool ne(TCHAR c1, TCHAR c2) { return !eq(c1, c2); }
	static bool lt(TCHAR c1, TCHAR c2) { return _totupper(c1) <  _totupper(c2); }
	static int compare(TCHAR const *s1, TCHAR const *s2, size_t n)
	{
		while (n-- != 0)
		{
			if (lt(*s1, *s2)) { return -1; }
			if (lt(*s2, *s1)) { return +1; }
			++s1; ++s2;
		}
		return 0;
	}
	static TCHAR const *find(TCHAR const *s, size_t n, TCHAR a)
	{
		while (n > 0 && ne(*s, a)) { ++s; --n; }
		return s;
	}
};

template<class It1, class It2, class Tr>
inline bool wildcard(It1 patBegin, It1 const patEnd, It2 strBegin, It2 const strEnd, Tr const &tr = std::char_traits<typename std::iterator_traits<It2>::value_type>())
{
	(void)tr;
	if (patBegin == patEnd) { return strBegin == strEnd; }
	//http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
	It2 s(strBegin);
	It1 p(patBegin);
	bool star = false;

loopStart:
	for (s = strBegin, p = patBegin; s != strEnd && p != patEnd; ++s, ++p)
	{
		if (tr.eq(*p, _T('*')))
		{
			star = true;
			strBegin = s, patBegin = p;
			if (++patBegin == patEnd)
			{ return true; }
			goto loopStart;
		}
		else if (tr.eq(*p, _T('?')))
		{
			if (tr.eq(*s, _T('.')))
			{ goto starCheck; }
		}
		else
		{
			if (!tr.eq(*s, *p))
			{ goto starCheck; }
		}
	}
	while (p != patEnd && tr.eq(*p, _T('*'))) { ++p; }
	return p == patEnd && s == strEnd;

starCheck:
	if (!star) { return false; }
	strBegin++;
	goto loopStart;
}

template<class R1, class R2, class Tr>
inline bool wildcard(R1 const &pat, R2 const &str, Tr const &tr = std::char_traits<typename boost::range_value<R1>::type>())
{
	return wildcard(pat.begin(), pat.end(), str.begin(), str.end(), tr);
}

class WildcardMatcher : public Matcher
{
	std::basic_string<TCHAR> pattern;
	tchar_ci_traits traits;
public:
	WildcardMatcher(boost::iterator_range<TCHAR const *> const &pattern)
		: pattern(pattern.begin(), pattern.end()) { }
	bool operator()(boost::iterator_range<TCHAR const *> const path, boost::iterator_range<TCHAR const *> const streamName)
	{
		return streamName.empty() ? wildcard(pattern, path, traits) : wildcard(pattern, boost::join(path, boost::join(boost::as_literal(_T(":")), streamName)), traits);
	}
};

class RegexMatcher : public Matcher
{
	boost::xpressive::basic_regex<TCHAR const *> regex;
public:
	RegexMatcher(boost::iterator_range<TCHAR const *> const &pattern)
		: regex(boost::xpressive::basic_regex<TCHAR const *>::compile(
			pattern,
			boost::xpressive::regex_constants::icase |
			boost::xpressive::regex_constants::optimize |
			boost::xpressive::regex_constants::nosubs |
			boost::xpressive::regex_constants::single_line)) { }
	bool operator()(boost::iterator_range<TCHAR const *> const path, boost::iterator_range<TCHAR const *> const streamName)
	{
		if (!streamName.empty()) { throw std::logic_error("streamName must be empty!"); }
		return boost::xpressive::regex_match(path, regex);
	}
};

Matcher *Matcher::compile(boost::iterator_range<TCHAR const *> const pattern, bool regex)
{
	if (regex)
	{
		try { return new RegexMatcher(pattern); }
		catch (boost::xpressive::regex_error const &ex)
		{ throw std::domain_error(ex.what()); }
	}
	else { return new WildcardMatcher(pattern); }
}
