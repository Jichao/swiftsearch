#pragma once

#include <tchar.h>

#include <boost/range/iterator_range.hpp>

class Matcher
{
public:
	virtual ~Matcher() { }
	virtual bool operator()(boost::iterator_range<TCHAR const *> const path, boost::iterator_range<TCHAR const *> const streamName) = 0;

	static Matcher *compile(boost::iterator_range<TCHAR const *> const pattern, bool regex = false);
};
