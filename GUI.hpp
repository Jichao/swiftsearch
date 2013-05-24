#pragma once

#include <stdarg.h>
#include <tchar.h>

class CMainDlgBase
{
public:
	virtual ~CMainDlgBase() { }
	virtual intptr_t operator()(uintptr_t hWndParent) = 0;
	static CMainDlgBase *create(HANDLE const hEvent);
};

TCHAR const *GetAnyErrorText(unsigned long errorCode, va_list* pArgList = NULL);

EXTERN_C NTSYSAPI NTSTATUS NTAPI RtlSystemTimeToLocalTime(IN LARGE_INTEGER const *SystemTime, OUT PLARGE_INTEGER LocalTime);
EXTERN_C NTSYSAPI NTSTATUS NTAPI RtlLocalTimeToSystemTime(IN LARGE_INTEGER const *LocalTime, OUT PLARGE_INTEGER SystemTime);
LONGLONG RtlSystemTimeToLocalTime(LONGLONG systemTime);
LONGLONG RtlLocalTimeToSystemTime(LONGLONG localTime);
LPCTSTR SystemTimeToString(LONGLONG systemTime, LPTSTR buffer, size_t cchBuffer);
std::basic_string<TCHAR> &GetPath(class NtfsIndex const &index, unsigned long segmentNumber, std::basic_string<TCHAR> &path);