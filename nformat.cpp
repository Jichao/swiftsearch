#include "stdafx.h"
#include "nformat.hpp"

std::locale get_numpunct_locale(std::locale const &loc)
{
	std::locale result(loc);
#if defined(_MSC_VER) && defined(_ADDFAC)
	std::_ADDFAC(result, new std::numpunct<TCHAR>());
#else
	result = std::locale(locale, new std::numpunct<TCHAR>());
#endif
	return result;
}