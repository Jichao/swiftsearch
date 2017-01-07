#include "stdafx.h"
#include "utils.h"
#include "resource.h"
#include "nformat.hpp"
#include "path.hpp"
#include "BackgroundWorker.hpp"
#include "QueryFileLayout.h"
#include "ShellItemIDList.hpp"
#include "CModifiedDialogImpl.hpp"
#include "ntfsindex.h"
#include "ProgressDialog.h"
#include "overlap.h"
#include "maindlg.h"

WTL::CAppModule _Module;

bool runx64(DWORD* pexitCode)
{
    if (!IsDebuggerPresent()) {
        HMODULE hKernel32 = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&GetSystemInfo),
                          &hKernel32);
        typedef BOOL(WINAPI *PIsWow64Process)(IN HANDLE hProcess, OUT PBOOL Wow64Process);
        PIsWow64Process IsWow64Process = NULL;
        BOOL isWOW64 = FALSE;
        if (hKernel32 != NULL
                && (IsWow64Process = reinterpret_cast<PIsWow64Process>(GetProcAddress(hKernel32,
                                     "IsWow64Process"))) != NULL && IsWow64Process(GetCurrentProcess(), &isWOW64) && isWOW64) {
            HRSRC hRsrs = FindResource(NULL, _T("X64"), _T("BINARY"));
            LPVOID pBinary = LockResource(LoadResource(NULL, hRsrs));
            if (pBinary) {
                std::basic_string<TCHAR> tempDir(32 * 1024, _T('\0'));
                tempDir.resize(GetTempPath(static_cast<DWORD>(tempDir.size()), &tempDir[0]));
                if (!tempDir.empty()) {
                    std::basic_string<TCHAR> fileName = tempDir +
                                                        _T("SwiftSearch64_{3CACE9B1-EF40-4a3b-B5E5-3447F6A1E703}.exe");
                    struct Deleter {
                        std::basic_string<TCHAR> file;
                        ~Deleter()
                        {
                            if (!this->file.empty()) {
                                _tunlink(this->file.c_str());
                            }
                        }
                    } deleter;
                    bool success;
                    {
                        std::filebuf file;
                        std::ios_base::openmode const openmode = std::ios_base::out | std::ios_base::trunc |
                                std::ios_base::binary;
#if defined(_CPPLIB_VER)
                        success = !!file.open(fileName.c_str(), openmode);
#else
                        std::string fileNameChars;
                        std::copy(fileName.begin(), fileName.end(), std::inserter(fileNameChars,
                                  fileNameChars.end()));
                        success = !!file.open(fileNameChars.c_str(), openmode);
#endif
                        if (success) {
                            deleter.file = fileName;
                            file.sputn(static_cast<char const *>(pBinary),
                                       static_cast<std::streamsize>(SizeofResource(NULL, hRsrs)));
                            file.close();
                        }
                    }
                    if (success) {
                        STARTUPINFO si = { sizeof(si) };
                        GetStartupInfo(&si);
                        PROCESS_INFORMATION pi;
                        HANDLE hJob = CreateJobObject(NULL, NULL);
                        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits = { { { 0 }, { 0 }, JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE } };
                        if (hJob != NULL
                                && SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobLimits,
                                                           sizeof(jobLimits))
                                && AssignProcessToJobObject(hJob, GetCurrentProcess())) {
                            if (CreateProcess(fileName.c_str(), GetCommandLine(), NULL, NULL, FALSE,
                                              CREATE_PRESERVE_CODE_AUTHZ_LEVEL | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, NULL,
                                              NULL, &si, &pi)) {
                                jobLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                                SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobLimits,
                                                        sizeof(jobLimits));
                                if (ResumeThread(pi.hThread) != -1) {
                                    WaitForSingleObject(pi.hProcess, INFINITE);
                                    DWORD exitCode = 0;
                                    GetExitCodeProcess(pi.hProcess, &exitCode);
									*pexitCode = exitCode;
									return true;
                                } else {
                                    TerminateProcess(pi.hProcess, GetLastError());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
	return false;
}

int _tmain(int argc, TCHAR* argv[])
{
	winnt::init();
	DWORD exitCode = 0;
	if (runx64(&exitCode))
		return exitCode;

    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE,
                                _T("Local\\SwiftSearch.{CB77990E-A78F-44dc-B382-089B01207F02}"));
    if (hEvent != NULL && GetLastError() != ERROR_ALREADY_EXISTS) {
        typedef long(WINAPI *PNtSetTimerResolution)(unsigned long DesiredResolution,
                bool SetResolution, unsigned long *CurrentResolution);
        if (PNtSetTimerResolution const NtSetTimerResolution =
                    reinterpret_cast<PNtSetTimerResolution>(GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")),
                            "NtSetTimerResolution"))) {
            unsigned long prev;
            NtSetTimerResolution(1, true, &prev);
        }
        (void) argc;
        (void) argv;
        // pi();
        HINSTANCE const hInstance = GetModuleHandle(NULL);
        __if_exists(_Module) {
            _Module.Init(NULL, hInstance);
        }
        {
            WTL::CMessageLoop msgLoop;
            _Module.AddMessageLoop(&msgLoop);
            CMainDlg wnd(hEvent);
            wnd.Create(reinterpret_cast<HWND>(NULL), NULL);
            wnd.ShowWindow(SW_SHOWDEFAULT);
            msgLoop.Run();
            _Module.RemoveMessageLoop();
        }
        __if_exists(_Module) {
            _Module.Term();
        }
        return 0;
    } else {
        AllowSetForegroundWindow(ASFW_ANY);
        PulseEvent(
            hEvent);  // PulseThread() is normally unreliable, but we don't really care here...
        return GetLastError();
    }
}

int __stdcall _tWinMain(HINSTANCE const hInstance, HINSTANCE /*hPrevInstance*/,
                        LPTSTR /*lpCmdLine*/, int nShowCmd)
{
    (void) hInstance;
    (void) nShowCmd;
    return _tmain(__argc, __targv);
}

