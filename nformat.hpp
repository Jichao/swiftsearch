#pragma once

#ifndef NFORMAT_HPP
#define NFORMAT_HPP

#include <tchar.h>

#include <locale>
#include <string>
#include <sstream>

std::locale get_numpunct_locale(std::locale const &loc = std::locale(""))
{
	std::locale result(loc);
#if defined(_MSC_VER) && defined(_ADDFAC)
	std::_ADDFAC(result, new std::numpunct<TCHAR>());
#else
	result = std::locale(locale, new std::numpunct<TCHAR>());
#endif
	return result;
}

template<class T>
std::basic_string<TCHAR> nformat(T v, std::locale const &loc = std::locale(""), bool is_numpunct_locale = false)
{
	std::basic_ostringstream<TCHAR> ss;
	ss.imbue(is_numpunct_locale ? loc : get_numpunct_locale(loc));
	ss << v;
	return ss.str();
}

#if defined(_MSC_VER) && !defined(_WIN64) && (!defined(_CPPLIB_VER) || _CPPLIB_VER < 403)
template<class V>
std::basic_string<TCHAR> nformat64(V v, std::locale const &loc = std::locale(""), bool is_numpunct_locale = false)
{
	struct SS : public std::basic_ostringstream<TCHAR>
	{
		struct NumPunctFacet : public _Nput
		{
			typedef TCHAR _E;
			typedef std::ostreambuf_iterator<_E, std::char_traits<_E> > _OI;
			static char *__cdecl _Ifmt(char *_Fmt, const char *_Spec, ios_base::fmtflags _Fl)
			{
				char *_S = _Fmt;
				*_S++ = '%';
				if (_Fl & ios_base::showpos)
				{ *_S++ = '+'; }
				if (_Fl & ios_base::showbase)
				{ *_S++ = '#'; }
				*_S++ = _Spec[0];
				*_S++ = _Spec[1];
				*_S++ = _Spec[2];
				*_S++ = _Spec[3];
				*_S++ = _Spec[4];
				ios_base::fmtflags _Bfl = _Fl & ios_base::basefield;
				*_S++ = _Bfl == ios_base::oct ? 'o'
					: _Bfl != ios_base::hex ? _Spec[5]      // 'd' or 'u'
					: _Fl & ios_base::uppercase ? 'X' : 'x';
				*_S = '\0';
				return (_Fmt);
			}
			_OI do_put(_OI _F, ios_base& _X, _E _Fill, __int64 _V) const
			{
				char _Buf[2 * _MAX_INT_DIG], _Fmt[12];
				return (_Iput(_F, _X, _Fill, _Buf, sprintf(_Buf, _Ifmt(_Fmt, "I64lld", _X.flags()), _V)));
			}
			_OI do_put(_OI _F, ios_base& _X, _E _Fill, unsigned __int64 _V) const
			{
				char _Buf[2 * _MAX_INT_DIG], _Fmt[12];
				return (_Iput(_F, _X, _Fill, _Buf, sprintf(_Buf, _Ifmt(_Fmt, "I64llu", _X.flags()), _V)));
			}
		};
		SS &operator <<(V _X)
		{
			iostate _St = goodbit;
			const sentry _Ok(*this);
			if (_Ok)
			{
				const _Nput& _Fac = _USE(getloc(), _Nput);
				_TRY_IO_BEGIN
					if (static_cast<NumPunctFacet const &>(_Fac).do_put(_Iter(rdbuf()), *this, fill(), _X).failed())
					{
						_St |= badbit;
					}
				_CATCH_IO_END
			}
			setstate(_St);
			return *this;
		}
	} ss;
	ss.imbue(is_numpunct_locale ? loc : get_numpunct_locale(loc));
	ss << v;
	return ss.str();
}

std::basic_string<TCHAR> nformat(         long long v, std::locale const &loc = std::locale(""), bool is_numpunct_locale = false)
{ return nformat64(v, loc, is_numpunct_locale); }

std::basic_string<TCHAR> nformat(unsigned long long v, std::locale const &loc = std::locale(""), bool is_numpunct_locale = false)
{ return nformat64(v, loc, is_numpunct_locale); }
#endif

#endif