#pragma once

#define _WIN32_WINNT 0x502
#include <SDKDDKVer.h>
#define _USE_MATH_DEFINES 1
#define _CRT_OBSOLETE_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#define _SCL_SECURE_NO_WARNINGS 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1
#define _STRALIGN_USE_SECURE_CRT 0
#define _ATL_NO_COM 1
#define _ATL_NO_AUTOMATIC_NAMESPACE 1
#define _WTL_NO_AUTOMATIC_NAMESPACE 1
#define _WTL_USE_VSSYM32 1
#define STRICT 1
#define NOMINMAX 1
#define BUILD_WINDOWS 1
#define BOOST_ALL_NO_LIB 1

#ifndef _DEBUG
#define _SECURE_SCL 0
#define _ITERATOR_DEBUG_LEVEL 0
#define __STDC_WANT_SECURE_LIB__ 0
#define _STRALIGN_USE_SECURE_CRT 0
#endif

#if defined(__STDC_WANT_SECURE_LIB__) && !__STDC_WANT_SECURE_LIB__
#if !(defined(_MSC_VER) && _MSC_VER <= 1400)
#define sprintf_s(buf, size, ...) sprintf(buf, __VA_ARGS__)
#define swprintf_s(buf, size, ...) swprintf(buf, size, __VA_ARGS__)
#define wcstok_s(tok, delim, ctx) wcstok(tok, delim)
#define wcstok_s(tok, delim, ctx) wcstok(tok, delim)
#define wcscpy_s(a, b, c) wcscpy(a, c)
#define wcscat_s(a, b, c) wcscat(a, c)
#define memcpy_s(a, b, c, d) memcpy(a, c, d)
#define memmove_s(a, b, c, d) memmove(a, c, d)
#define wmemcpy_s(a, b, c, d) wmemcpy(a, c, d)
#define wmemmove_s(a, b, c, d) wmemmove(a, c, d)
#define strcpy_s(a, b, c) strcpy(a, c)
#define strncpy_s(a, b, c, d) strncpy(a, c, d)
#define wcsncpy_s(a, b, c, d) wcsncpy(a, c, d)
#define vswprintf_s(a, b, c, d) _vstprintf(a, c, d)
#define strcat_s(a, b, c) strcat(a, c)
#define ATL_CRT_ERRORCHECK(A) ((A), 0)
#endif
#endif
