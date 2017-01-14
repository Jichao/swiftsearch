#include "stdafx.h"
#include "utils.h"
#include "path.hpp"
#include "ShellItemIDList.hpp"

LONGLONG RtlSystemTimeToLocalTime(LONGLONG systemTime)
{
    LARGE_INTEGER time2, localTime;
    time2.QuadPart = systemTime;
    long status = winnt::RtlSystemTimeToLocalTime(&time2, &localTime);
    if (status != 0) {
        RaiseException(status, 0, 0, NULL);
    }
    return localTime.QuadPart;
}

LPCTSTR SystemTimeToString(LONGLONG systemTime, LPTSTR buffer, size_t cchBuffer,
                           LPCTSTR dateFormat/* = NULL*/, LPCTSTR timeFormat/* = NULL*/, LCID lcid /*= GetThreadLocale()*/)
{
    LONGLONG time = RtlSystemTimeToLocalTime(systemTime);
    SYSTEMTIME sysTime = { 0 };
    if (FileTimeToSystemTime(&reinterpret_cast<FILETIME &>(time), &sysTime)) {
        size_t const cchDate = static_cast<size_t>(GetDateFormat(lcid, 0, &sysTime, dateFormat,
                               &buffer[0], static_cast<int>(cchBuffer)));
        if (cchDate > 0) {
            // cchDate INCLUDES null-terminator
            buffer[cchDate - 1] = _T(' ');
            size_t const cchTime = static_cast<size_t>(GetTimeFormat(lcid, 0, &sysTime, timeFormat,
                                   &buffer[cchDate], static_cast<int>(cchBuffer - cchDate)));
            buffer[cchDate + cchTime - 1] = _T('\0');
        } else {
            memset(&buffer[0], 0, sizeof(buffer[0]) * cchBuffer);
        }
    } else {
        *buffer = _T('\0');
    }
    return buffer;
}


std::tstring NormalizePath(std::tstring const &path)
{
    std::tstring result;
    bool wasSep = false;
    bool isCurrentlyOnPrefix = true;
    for (size_t i = 0; i < path.size(); i++) {
        TCHAR const &c = path[i];
        isCurrentlyOnPrefix &= isdirsep(c);
        if (isCurrentlyOnPrefix || !wasSep || !isdirsep(c)) {
            result += c;
        }
        wasSep = isdirsep(c);
    }
    if (!isrooted(result.begin(), result.end())) {
        std::tstring currentDir(32 * 1024, _T('\0'));
        currentDir.resize(GetCurrentDirectory(static_cast<DWORD>(currentDir.size()),
                                              &currentDir[0]));
        adddirsep(currentDir);
        result = currentDir + result;
    }
    return result;
}

std::tstring GetDisplayName(HWND hWnd, const std::tstring &path, DWORD shgdn)
{
    ATL::CComPtr<IShellFolder> desktop;
    STRRET ret;
    LPITEMIDLIST shidl;
    ATL::CComBSTR bstr;
    ULONG attrs = SFGAO_ISSLOW | SFGAO_HIDDEN;
    std::tstring result = (
                              SHGetDesktopFolder(&desktop) == S_OK
                              && desktop->ParseDisplayName(hWnd, NULL,
                                      path.empty() ? NULL : const_cast<LPWSTR>(path.c_str()), NULL, &shidl, &attrs) == S_OK
                              && (attrs & SFGAO_ISSLOW) == 0
                              && desktop->GetDisplayNameOf(shidl, shgdn, &ret) == S_OK
                              && StrRetToBSTR(&ret, shidl, &bstr) == S_OK
                          ) ? static_cast<LPCTSTR>(bstr) : std::tstring(basename(path.begin(), path.end()),
                                  path.end());
    return result;
}

void CheckAndThrow(int const success)
{
    if (!success) {
        RaiseException(GetLastError(), 0, 0, NULL);
    }
}

LPTSTR GetAnyErrorText(DWORD errorCode, va_list* pArgList/* = NULL*/)
{
    static TCHAR buffer[1 << 15];
    ZeroMemory(buffer, sizeof(buffer));
    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | (pArgList == NULL ?
                       FORMAT_MESSAGE_IGNORE_INSERTS : 0), NULL, errorCode, 0, buffer,
                       sizeof(buffer) / sizeof(*buffer), pArgList)) {
        if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | (pArgList == NULL ?
                           FORMAT_MESSAGE_IGNORE_INSERTS : 0), GetModuleHandle(_T("NTDLL.dll")), errorCode, 0,
                           buffer, sizeof(buffer) / sizeof(*buffer), pArgList)) {
            _stprintf(buffer, _T("%#x"), errorCode);
        }
    }
    return buffer;
}

unsigned int get_cluster_size(void *const volume)
{
	winnt::IO_STATUS_BLOCK iosb;
	winnt::FILE_FS_SIZE_INFORMATION info = {};
	if (winnt::NtQueryVolumeInformationFile(volume, &iosb, &info, sizeof(info), 3)) {
		SetLastError(ERROR_INVALID_FUNCTION), CheckAndThrow(false);
	}
	return info.BytesPerSector * info.SectorsPerAllocationUnit;
}

void read(void *const file, uint64_t const offset, void *const buffer, size_t const size, HANDLE const event_handle /*= NULL*/)
{
	if (!event_handle || event_handle == INVALID_HANDLE_VALUE) {
		read(file, offset, buffer, size, Handle(CreateEvent(NULL, TRUE, FALSE, NULL)));
	}
	else {
		OVERLAPPED overlapped = {};
		overlapped.hEvent = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>
			(static_cast<void *>(event_handle)) | 1);
		overlapped.Offset = static_cast<unsigned int>(offset);
		overlapped.OffsetHigh = static_cast<unsigned int>(offset >> (CHAR_BIT * sizeof(
			overlapped.Offset)));
		unsigned long nr;
		CheckAndThrow(ReadFile(file, buffer, static_cast<unsigned long>(size), &nr, &overlapped)
			|| (GetLastError() == ERROR_IO_PENDING
			&& GetOverlappedResult(file, &overlapped, &nr, TRUE)));
	}
}

std::vector<std::pair<uint64_t, int64_t> > get_mft_retrieval_pointers(
    void *const volume, TCHAR const path[], int64_t *const size,
    int64_t const mft_start_lcn, unsigned int const file_record_size)
{
    (void) mft_start_lcn;
    (void) file_record_size;
    typedef std::vector<std::pair<uint64_t, int64_t> > Result;
    Result result;
    Handle handle;
    {
        Handle root_dir;
        {
            uint64_t root_dir_id = 0x0005000000000005;
            winnt::UNICODE_STRING us = { sizeof(root_dir_id), sizeof(root_dir_id), reinterpret_cast<wchar_t *>(&root_dir_id) };
            winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), volume, &us };
            winnt::IO_STATUS_BLOCK iosb;
            unsigned long const error = winnt::RtlNtStatusToDosError(winnt::NtOpenFile(
                                            &root_dir.value, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &iosb,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            0x00002000 /* FILE_OPEN_BY_FILE_ID */ | 0x00000020 /* FILE_SYNCHRONOUS_IO_NONALERT */));
            if (error) {
                SetLastError(error);
                CheckAndThrow(!error);
            }
        }
        {
            size_t const cch = path ? std::char_traits<TCHAR>::length(path) : 0;
            winnt::UNICODE_STRING us = { static_cast<unsigned short>(cch * sizeof(*path)), static_cast<unsigned short>(cch * sizeof(*path)), const_cast<TCHAR *>(path) };
            winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), root_dir, &us };
            winnt::IO_STATUS_BLOCK iosb;
            unsigned long const error = winnt::RtlNtStatusToDosError(winnt::NtOpenFile(&handle.value,
                                        FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &iosb,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        0x00200000 /* FILE_OPEN_REPARSE_POINT */ |
                                        0x00000020 /* FILE_SYNCHRONOUS_IO_NONALERT */));
            if (error) {
                SetLastError(error);
                CheckAndThrow(!error);
            }
        }
    }
    result.resize(1 + (sizeof(RETRIEVAL_POINTERS_BUFFER) - 1) / sizeof(Result::value_type));
    STARTING_VCN_INPUT_BUFFER input = {};
    BOOL success;
    for (unsigned long nr;
            !(success = DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &input, sizeof(input),
                                        &*result.begin(), static_cast<unsigned long>(result.size()) * sizeof(*result.begin()),
                                        &nr, NULL), success) && GetLastError() == ERROR_MORE_DATA;) {
        size_t const n = result.size();
        Result(/* free old memory */).swap(result);
        Result(n * 2).swap(result);
    }
    CheckAndThrow(success);
    if (size) {
        LARGE_INTEGER large_size;
        CheckAndThrow(GetFileSizeEx(handle, &large_size));
        *size = large_size.QuadPart;
    }
    result.erase(result.begin() + 1 + reinterpret_cast<unsigned long const &>
                 (*result.begin()), result.end());
    result.erase(result.begin(), result.begin() + 1);
    return result;
}

unsigned long get_num_threads()
{
	unsigned long num_threads = 0;
#ifdef _OPENMP
#pragma omp parallel
#pragma omp atomic
	++num_threads;
#else
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	if (int const n = _ttoi((_T("OMP_NUM_THREADS")))) {
		num_threads = static_cast<int>(n);
	}
	else {
		num_threads = sysinfo.dwNumberOfProcessors;
	}
#endif
	return num_threads;
}

DWORD WINAPI SHOpenFolderAndSelectItemsThread(IN LPVOID lpParameter)
{
	std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > >
		p(
		static_cast<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > *>
		(lpParameter));
	// This is in a separate thread because of a BUG:
	// Try this with RmMetadata:
	// 1. Double-click it.
	// 2. Press OK when the error comes up.
	// 3. Now you can't access the main window, because SHOpenFolderAndSelectItems() hasn't returned!
	// So we put this in a separate thread to solve that problem.

	CoInit coInit;
	std::vector<LPCITEMIDLIST> relative_item_ids(p->second.size());
	for (size_t i = 0; i < p->second.size(); ++i) {
		relative_item_ids[i] = ILFindChild(p->first.first, p->second[i]);
	}
	return SHOpenFolderAndSelectItems(p->first.first,
		static_cast<UINT>(relative_item_ids.size()),
		relative_item_ids.empty() ? NULL : &relative_item_ids[0], 0);
}

void remove_path_stream_and_trailing_sep(std::tstring &path)
    {
        for (;;) {
            size_t colon_or_sep = path.find_last_of(_T("\\:"));
            if (!~colon_or_sep || path[colon_or_sep] != _T(':')) {
                break;
            }
            path.erase(colon_or_sep, path.size() - colon_or_sep);
        }
        while (!path.empty() && isdirsep(*(path.end() - 1))
                && path.find_first_of(_T('\\')) != path.size() - 1) {
            path.erase(path.end() - 1);
        }
        if (!path.empty() && *(path.end() - 1) == _T('.') && (path.size() == 1
                || isdirsep(*(path.end() - 2)))) {
            path.erase(path.end() - 1);
        }
    }



