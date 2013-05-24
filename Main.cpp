#include "stdafx.h"
#include "targetver.h"

#include <io.h>

#include <fstream>
#include <sstream>

#include <atlbase.h>
#include <atlapp.h>
#include <atluser.h>

#include <Windows.h>
#include <ProvExce.h>

#include "NtfsIndex.hpp"
#include "GUI.hpp"
#include "NtObjectMini.hpp"
#include "path.hpp"

ATL::CComModule _Module;

TCHAR const *GetAnyErrorText(unsigned long errorCode, va_list* pArgList)
{
	static TCHAR buffer[1 << 15];
	ZeroMemory(buffer, sizeof(buffer));
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | (pArgList == NULL ? FORMAT_MESSAGE_IGNORE_INSERTS : 0), NULL, errorCode, 0, buffer, sizeof(buffer) / sizeof(*buffer), pArgList))
	{
		if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | (pArgList == NULL ? FORMAT_MESSAGE_IGNORE_INSERTS : 0), GetModuleHandle(_T("NTDLL.dll")), errorCode, 0, buffer, sizeof(buffer) / sizeof(*buffer), pArgList))
		{ _stprintf(buffer, _T("%#x"), errorCode); }
	}
	return buffer;
}

#if 0
#include <iostream>
LONG WINAPI MyUnhandledExceptionFilter(IN struct _EXCEPTION_POINTERS* ExceptionInfo)
{
	struct MyStackWalker : StackWalker
	{
		MyStackWalker() : StackWalker() { }
		static void MyStrCpy(char* szDest, size_t nMaxDestSize, const char* szSrc)
		{
			if (nMaxDestSize <= 0) return;
			if (strlen(szSrc) < nMaxDestSize)
			{
				strcpy(szDest, szSrc);
			}
			else
			{
				strncpy(szDest, szSrc, nMaxDestSize);
				szDest[nMaxDestSize-1] = 0;
			}
		}
		void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion) { }
		void OnOutput(LPCSTR buffer)
		{
			std::cerr << buffer;
		}
	} sw;
	sw.ShowCallstack(GetCurrentThread(), ExceptionInfo->ContextRecord);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

int run(HINSTANCE hInstance, int nShowCmd)
{
	//SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
#if !defined(_WIN64) //&& defined(NDEBUG) && NDEBUG
	if (!IsDebuggerPresent())
	{
		HMODULE hKernel32 = NULL;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&GetSystemInfo), &hKernel32);
		typedef BOOL (WINAPI *PIsWow64Process)(IN HANDLE hProcess, OUT PBOOL Wow64Process);
		PIsWow64Process IsWow64Process = NULL;
		BOOL isWOW64 = FALSE;
		if (hKernel32 != NULL && (IsWow64Process = reinterpret_cast<PIsWow64Process>(GetProcAddress(hKernel32, "IsWow64Process"))) != NULL && IsWow64Process(GetCurrentProcess(), &isWOW64) && isWOW64)
		{
			HRSRC hRsrs = FindResource(NULL, _T("X64"), _T("BINARY"));
			LPVOID pBinary = LockResource(LoadResource(NULL, hRsrs));
			if (pBinary)
			{
				std::basic_string<TCHAR> tempDir(32 * 1024, _T('\0'));
				tempDir.resize(GetTempPath(int_cast<DWORD>(tempDir.size()), &tempDir[0]));
				if (!tempDir.empty())
				{
					std::basic_string<TCHAR> fileName = tempDir + _T("SwiftSearch64_{3CACE9B1-EF40-4a3b-B5E5-3447F6A1E703}.exe");
					struct Deleter
					{
						std::basic_string<TCHAR> file;
						~Deleter() { if (!this->file.empty()) { _tunlink(this->file.c_str()); } }
					} deleter;
					std::string fileNameChars;
					std::copy(fileName.begin(), fileName.end(), std::inserter(fileNameChars, fileNameChars.end()));
					std::ofstream file(fileNameChars.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
					if (!file.bad() && !file.fail())
					{
						deleter.file = fileName;
						file.write(static_cast<char const *>(pBinary), SizeofResource(NULL, hRsrs));
						file.flush();
						file.close();
						STARTUPINFO si = { sizeof(si) };
						GetStartupInfo(&si);
						PROCESS_INFORMATION pi;
						HANDLE hJob = CreateJobObject(NULL, NULL);
						JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits = { { { 0 }, { 0 }, JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE } };
						if (hJob != NULL
							&& SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobLimits, sizeof(jobLimits))
							&& AssignProcessToJobObject(hJob, GetCurrentProcess()))
						{
							if (CreateProcess(fileName.c_str(), GetCommandLine(), NULL, NULL, FALSE, CREATE_PRESERVE_CODE_AUTHZ_LEVEL | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi))
							{
								if (ResumeThread(pi.hThread) != -1)
								{
									WaitForSingleObject(pi.hProcess, INFINITE);
									DWORD exitCode = 0;
									GetExitCodeProcess(pi.hProcess, &exitCode);
									return exitCode;
								}
								else { TerminateProcess(pi.hProcess, GetLastError()); }
							}
						}
					}
				}
			}
			/* continue running in x86 mode... */
		}
	}
#endif
	UNREFERENCED_PARAMETER(nShowCmd);

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, _T("Local\\SwiftSearch.{CB77990E-A78F-44dc-B382-089B01207F02}"));
	if (hEvent != NULL && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		int result = 0;
		try { winnt::NtProcess::RtlAdjustPrivilege(SeManageVolumePrivilege, true); }
		catch (CStructured_Exception &ex) { result = ex.GetSENumber(); }
		if (result == 0)
		{
			__if_exists(_Module) { _Module.Init(NULL, hInstance); }
#if defined(NDEBUG)
			try
#endif
			{
				std::auto_ptr<CMainDlgBase> dlg(CMainDlgBase::create(hEvent));
				(*dlg)(NULL);
			}
#if defined(NDEBUG)
			catch (CStructured_Exception &ex)
			{
				std::basic_stringstream<TCHAR> ss;
				ss
					<< _T("A fatal error caused the application to crash.\r\nThe following information may be useful for diagnosing the problem:\r\n----------------\r\nError code: ")
					<< ex.GetSENumber()
					<< _T("\r\nError message: ")
					<< GetAnyErrorText(ex.GetSENumber());
				WTL::AtlMessageBox(NULL, ss.str().c_str(), _T("Fatal Error"), MB_OK | MB_ICONERROR);
				result = static_cast<int>(ex.GetSENumber());
			}
#endif
			__if_exists(_Module) { _Module.Term(); }
		}
		else { WTL::AtlMessageBox(NULL, _T("This program requires administrative privileges in order to run.\r\n\r\nDetails: \r\nThis program needs direct read access to the file system, which requires the \"manage volume\" privilege.\r\nBecause this bypasses many security features, this privilege is typically only granted to administrators.\r\nContact your local administrator to obtain this privilege."), _T("Administrative Privileges Required"), MB_ICONERROR | MB_OK); }
		return result;
	}
	else
	{
		AllowSetForegroundWindow(ASFW_ANY);
		PulseEvent(hEvent);  // PulseThread() is normally unreliable, but we don't really care here...
		return GetLastError();
	}
}

#if defined(NDEBUG)
#pragma comment(linker, "/Subsystem:Console,5.02")
#else
#pragma comment(linker, "/Subsystem:Console,5.02")
#endif

int _tmain(int argc, LPTSTR argv[])
{
	int r;
	if (argc > 1)
	{
		_setmode(_fileno(stdout), 0x40000 /*_O_U8TEXT*/);
		r = 0;
		std::vector<std::basic_string<TCHAR> > args;
		args.reserve(argc);
		std::copy(&argv[1], &argv[argc], std::back_inserter(args));
		enum
		{
		};
		std::vector<std::basic_string<TCHAR> > paths;
		std::vector<std::pair<int, long long> > date_filters;
		for (size_t i = 0; i < args.size(); i++)
		{
			std::basic_string<TCHAR> const &arg = args[i];
			if (!arg.empty() && (*arg.begin() == _T('/') || *arg.begin() == _T('-')))
			{
				size_t j;
				for (j = 0; j < arg.size(); j++)
				{
					if (j == 0 && arg[j] != _T('-') && arg[j] != _T('/') ||
						j == 1 && (arg[j - 1] != _T('-') || arg[j] != _T('-')) ||
						j > 1)
					{ break; }
				}
				size_t const key_start = j;
				while (j < arg.size() && arg[j] != _T(':') && arg[j] != _T('='))
				{ ++j; }
				std::basic_string<TCHAR> const key = arg.substr(key_start, j - key_start);
				std::basic_string<TCHAR> value = j < arg.size() && (arg[j] == _T(':') || arg[j] == _T('=')) ? arg.substr(++j) : std::basic_string<TCHAR>();
				if (key == _T("C") || key == _T("M") || key == _T("A"))
				{
					long long factor = 1000 * 1000 * 10;  // # of seconds
					if (!value.empty())
					{
						if (*value.begin() == _T('-'))
						{
							value.erase(value.begin());
							factor *= -1;
						}
						else if (*value.begin() == _T('+'))
						{ value.erase(value.begin()); }
					}
					if (!value.empty())
					{
						if (*(value.end() - 1) == _T('m'))
						{
							factor *= 60;
							value.erase(value.end() - 1);
						}
						else if (*(value.end() - 1) == _T('h'))
						{
							factor *= 60 * 60;
							value.erase(value.end() - 1);
						}
						else if (*(value.end() - 1) == _T('d'))
						{
							factor *= 60 * 60 * 24;
							value.erase(value.end() - 1);
						}
					}
					long long systemTime;
					GetSystemTimeAsFileTime(&reinterpret_cast<FILETIME &>(systemTime));
					systemTime -= _ttoi64(value.c_str()) * factor * (factor < 0 ? -1 : 1);
					date_filters.push_back(std::make_pair(key == _T("C") ? 1 : key == _T("M") ? 2 : key == _T("A") ? 3 : 0, factor < 0 ? -systemTime : systemTime));
				}
			}
			else
			{
				paths.push_back(arg);
			}
		}
		for (size_t i = 0; i < paths.size(); i++)
		{
			winnt::NtObject is_allowed_event(CreateEvent(NULL, TRUE, TRUE, NULL));
			unsigned long progress = 0;
			bool background = false;
			std::basic_string<TCHAR> volume_path = paths[i], name;
			volume_path.erase(trimdirsep(volume_path.begin(), volume_path.end()), volume_path.end());
			if (!volume_path.empty() && !isdirsep(*volume_path.begin()))
			{ volume_path.insert(0, _T("\\\\.\\")); }
			winnt::NtFile volume;
			try
			{
				std::basic_string<TCHAR> ntpath = volume_path.size() > 2 && (!isdirsep(volume_path[0]) || isdirsep(volume_path[1])) ? winnt::NtFile::RtlDosPathNameToNtPathName(volume_path.c_str()) : volume_path;
				volume = winnt::NtFile::NtOpenFile(winnt::ObjectAttributes(ntpath), winnt::Access::Read | winnt::Access::Synchronize, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0);
			}
			catch (CStructured_Exception &ex)
			{
				std::basic_string<TCHAR> msg = GetAnyErrorText(ex.GetSENumber(), NULL);
				while (!msg.empty() && *(msg.end() - 1) == _T('\n'))
				{
					msg.resize(msg.size() - 1);
				}
				_ftprintf(stderr, _T("Error 0x%X opening \"%s\": %s\n"), ex.GetSENumber(), volume_path.c_str(), msg.c_str());
				r = ex.GetSENumber();
			}

			// Example: SwiftSearch.exe /M:+20m /A:-10m D:\
			if (volume)
			{
				HANDLE const hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
				std::basic_string<TCHAR> path;
				std::auto_ptr<NtfsIndex> const index(NtfsIndex::create(volume, paths[i], is_allowed_event, &progress, &background));
				size_t const n = index->size();
				name.reserve(32 * 1024);
				path.reserve(32 * 1024);
				std::basic_string<TCHAR> str, buf1, buf2, buf3;
				for (size_t i = 0; i < n; i++)
				{
					NtfsIndex::CombinedRecord const &record = index->at(i);
					bool match = true;
					for (size_t j = 0; j < date_filters.size(); j++)
					{
						long long d;
						switch (date_filters[j].first)
						{
						case 1: d = record.second.first.first.first; break;
						case 2: d = record.second.first.first.second; break;
						case 3: d = record.second.first.second.first; break;
						default: continue;
						}
						long long const boundary = date_filters[j].second;
						match &= boundary >= 0 ? d <= boundary : d >= -boundary;
					}
					if (!match) { continue; }
					name.erase(name.begin(), name.end());
					path.erase(path.begin(), path.end());
					NtfsIndex::SegmentNumber const parent = index->get_name_by_record(record, name);
					GetPath(*index, parent, path);
					adddirsep(path);
					path += name;
					if (name != _T(".") && (record.second.first.second.second & FILE_ATTRIBUTE_DIRECTORY) != 0)
					{ adddirsep(path); }
					str.resize(1024 + path.size());
					buf1.resize(1024);
					buf2.resize(1024);
					buf3.resize(1024);
					str.resize(
						static_cast<size_t>(
						_stprintf(
						&*str.begin(),
						_T("%I64llu|%I64llu|%s|%s|%s|%.*s"),
						static_cast<unsigned long long>(record.second.second.second.second.first),
						static_cast<unsigned long long>(record.second.second.second.second.second),
						SystemTimeToString(record.second.first.first.first , &*buf1.begin(), buf1.size()),
						SystemTimeToString(record.second.first.first.second, &*buf2.begin(), buf2.size()),
						SystemTimeToString(record.second.first.second.first, &*buf3.begin(), buf3.size()),
						static_cast<int>(path.size()), path.c_str())));
					str.append(1, _T('\n'));
					unsigned long nw;
					if (!WriteConsole(hStdOut, str.c_str(), static_cast<unsigned int>(str.size()), &nw, NULL))
					{
						_tprintf(_T("%.*s"), static_cast<int>(str.size()), str.c_str());
					}
				}
			}
		}
	}
	else
	{
		STARTUPINFO si = { sizeof(si) };
		GetStartupInfo(&si);
		r = run(GetModuleHandle(NULL), si.wShowWindow);
	}
	return r;
}

int __stdcall _tWinMain(HINSTANCE const hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	(void)hInstance;
	(void)nShowCmd;
	return _tmain(__argc, __targv);
}

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void __cdecl __report_rangecheckfailure() { abort(); }

#if !defined(_NATIVE_WCHAR_T_DEFINED) && (defined(DDK_CTYPE_WCHAR_FIX) && DDK_CTYPE_WCHAR_FIX)
#include <iostream>
namespace std
{
	const ctype<wchar_t>::mask *ctype<wchar_t>::_Cltab = NULL;  // TODO: Fix
}
#endif
