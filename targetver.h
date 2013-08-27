#pragma once

#pragma warning(disable: 4201)  // [winioctl]      nameless struct/union
#pragma warning(disable: 4428)  // (My code)       universal-character-name encountered in source
#pragma warning(disable: 4510)  // [boost/pheonix] default constructor could not be generated
#pragma warning(disable: 4512)  // (My code)       assignment operator could not be generated
#pragma warning(disable: 4610)  // [boost/pheonix] user-defined constructor required
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x502
#endif
#define _WIN32_IE 0x600
#define _STRALIGN_USE_SECURE_CRT 0
#define _ATL_NO_COM 1
#define _ATL_NO_AUTOMATIC_NAMESPACE 1
#define _WTL_NO_AUTOMATIC_NAMESPACE 1
#define _WTL_USE_VSSYM32 1
#define _HAS_ITERATOR_DEBUGGING 0
#define BOOST_ALL_NO_LIB 1
#define BOOST_USE_WINDOWS_H 1  // Otherwise it tries to include <intrin.h> which gets annoying with DDK
//#define BOOST_RESULT_OF_USE_DECLTYPE 1
#define BOOST_TYPEOF_LIMIT_SIZE 16
#define BOOST_TYPEOF_MESSAGE 1
#define DDK_CTYPE_WCHAR_FIX 1
#define NTDDI_WIN8 0x06020000
#define LIMIT_FUNCTION_ARITY 8
#define INCL_WINSOCK_API_PROTOTYPES 0  // bind() conflicts with boost
#define NTOBJECT_NTFS 1
#if defined(_DEBUG)
#define _ITERATOR_DEBUG_LEVEL 1
#else
// #define _SECURE_SCL 0
#endif
#define _SCL_SECURE_NO_WARNINGS 1
