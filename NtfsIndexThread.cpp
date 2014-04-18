#include "stdafx.h"
#include "targetver.h"

#include "NtfsIndexThread.hpp"

#include <process.h>

#include <algorithm>
#include <iterator>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <Dbt.h>

#include "NtfsIndex.hpp"

#include "NtObjectMini.hpp"

TCHAR const *GetAnyErrorText(unsigned long errorCode, va_list* pArgList = NULL);

#ifdef _M_X64
extern "C" void *__cdecl _InterlockedExchangePointer(void *volatile *Destination, void *ExChange);
#pragma intrinsic(_InterlockedExchangePointer)
#else
extern "C" long __cdecl _InterlockedExchange(long volatile *Target, long Value);
#pragma intrinsic(_InterlockedExchange)
#undef _InterlockedExchangePointer
#define _InterlockedExchangePointer(dest, xchg) (void *)_InterlockedExchange((long volatile *)(dest), (long)(xchg))
#endif

void SetThreadName(char const *threadName, unsigned long threadID = static_cast<unsigned long>(-1))
{
#pragma pack(push, 8)
	struct
	{
		unsigned long dwType; // Must be 0x1000.
		char const *szName; // Pointer to name (in user addr space).
		unsigned long dwThreadID; // Thread ID (-1 = caller thread).
		unsigned long dwFlags; // Reserved for future use, must be zero.
	} info = { 0x1000, threadName, threadID, 0 };
#pragma pack(pop)
	__try { RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info); }
	__except (EXCEPTION_EXECUTE_HANDLER) { }
}

NtfsIndexThread::~NtfsIndexThread()
{
	InterlockedExchange(&reinterpret_cast<long volatile &>(this->_progress), static_cast<long>((std::numeric_limits<unsigned long>::max)()));
	this->_thread.NtWaitForSingleObject();
}

NtfsIndexThread::NtfsIndexThread(HWND const hWnd, unsigned int wndMessage, std::basic_string<TCHAR> const &drive)
	: _drive(drive), _event(CreateEvent(NULL, TRUE, TRUE, NULL)), _background(true), hWnd(hWnd), wndMessage(wndMessage), _progress(), _speed()
{
	struct Invoker
	{
		static unsigned int __stdcall invoke(void *me)
		{ return (*static_cast<NtfsIndexThread *>(me))(); }
	};
	unsigned int tid;
	*this->_thread.receive() = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &Invoker::invoke, this, CREATE_SUSPENDED, &tid));
	SetThreadPriority(this->_thread.get(), THREAD_PRIORITY_BELOW_NORMAL);
	ResumeThread(this->_thread.get());
}

void NtfsIndexThread::close()
{
	InterlockedExchange(&reinterpret_cast<long volatile &>(this->_progress), static_cast<long>(NtfsIndex::PROGRESS_CANCEL_REQUESTED));
	struct Helper
	{
		static void __stdcall cancel(ULONG_PTR p)
		{
			winnt::NtFile const volume =
				_InterlockedExchangePointer(&reinterpret_cast<void *volatile &>(reinterpret_cast<NtfsIndexThread *>(p)->_volume._handle), NULL);
			// throw CStructured_Exception(ERROR_CANCELLED, NULL);
		}
	};
	QueueUserAPC(&Helper::cancel, this->_thread.get(), (ULONG_PTR)this);
}

boost::shared_ptr<NtfsIndex> NtfsIndexThread::delete_index()
{
	boost::shared_ptr<NtfsIndex> const result = boost::atomic_exchange(&this->_index, boost::shared_ptr<NtfsIndex>());
	this->_speed = 0;
	this->_progress = 0;
	return result;
}

uintptr_t NtfsIndexThread::volume() const { return reinterpret_cast<uintptr_t>(this->_volume.get()); }
uintptr_t NtfsIndexThread::thread() const { return reinterpret_cast<uintptr_t>(this->_thread.get()); }
uintptr_t NtfsIndexThread::event() const { return reinterpret_cast<uintptr_t>(this->_event.get()); }
bool NtfsIndexThread::has_index() const { return !!this->_index; }
boost::shared_ptr<NtfsIndex> NtfsIndexThread::index() const { return this->_index; }

unsigned int NtfsIndexThread::operator()()
{
	boost::intrusive_ptr<NtfsIndexThread> const me = this;  // Keep a reference to myself, in case others release us
	// Sleep((37 * GetCurrentThreadId()) % 100);
	unsigned int r = 0;
	try
	{
		try
		{
			std::basic_string<TCHAR> ntPath = winnt::NtFile::RtlDosPathNameToNtPathName(this->_drive.c_str());
			while (!ntPath.empty() && ntPath[ntPath.size() - 1] == _T('\\')) { ntPath.resize(ntPath.size() - 1); }
			ntPath += _T("\\$Volume");
			winnt::NtFile volume = winnt::NtFile::NtOpenFile(
				ntPath,
				winnt::Access::QueryAttributes | winnt::Access::Read | winnt::Access::Synchronize,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				0);
			volume.swap(this->_volume);
		}
		catch (CStructured_Exception &ex) { r = ex.GetSENumber(); }
		if (this->_volume.get())
		{
			if (IsDebuggerPresent())
			{
				std::string s = "Worker thread for drive: ";
				for (size_t i = 0; i < this->_drive.size(); i++)
				{ s.append(1, static_cast<char>(this->_drive[i])); }
				SetThreadName(s.c_str());
			}

			DEV_BROADCAST_HANDLE dev = { sizeof(dev), DBT_DEVTYP_HANDLE, 0, this->_volume.get(), reinterpret_cast<HDEVNOTIFY>(this) };
			dev.dbch_hdevnotify = RegisterDeviceNotification(this->hWnd, &dev, DEVICE_NOTIFY_WINDOW_HANDLE);

			for (; ; )
			{
				FILETIME creationTime = { }, exitTime = { }, kernelTime1 = { }, kernelTime2 = { }, userTime2 = { }, userTime1 = { };
				GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime1, &userTime1);
					
				boost::atomic_exchange(
					&this->_index,
					boost::shared_ptr<NtfsIndex>(new NtfsIndex(this->_volume, this->_drive, this->_event, &this->_progress, &this->_speed, &this->_background)));

				if (!GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime2, &userTime2)) { break; }
				LARGE_INTEGER q1 = { }, q2 = { };
				q1.LowPart = userTime1.dwLowDateTime + kernelTime1.dwLowDateTime;
				q1.HighPart = userTime1.dwHighDateTime + kernelTime1.dwHighDateTime;
				q2.LowPart = userTime2.dwLowDateTime + kernelTime2.dwLowDateTime;
				q2.HighPart = userTime2.dwHighDateTime + kernelTime2.dwHighDateTime;
				unsigned long const sleepInterval = 100;  // The program can't exit while sleeping! So keep this value low.
				using std::max;
				using std::min;
				// if (!IsDebuggerPresent())
				{
					for (long long nMillisToSleep = min(20 * 60 * 1000, max(10 * 60 * 1000, 80 * (q2.QuadPart - q1.QuadPart) / 10000));
						nMillisToSleep > 0;
						nMillisToSleep -= sleepInterval)
					{
						if (!this->_index)
						{
							break;
						}
						if (this->_progress == NtfsIndex::PROGRESS_CANCEL_REQUESTED)
						{
							throw CStructured_Exception(ERROR_CANCELLED, NULL);
						}
						while (SleepEx(sleepInterval, TRUE) == WAIT_IO_COMPLETION)
						{
						}
					}
				}
			}
		}
	}
	catch (CStructured_Exception &ex)
	{
		if (ex.GetSENumber() != ERROR_CANCELLED &&
			ex.GetSENumber() != ERROR_INVALID_HANDLE &&
			ex.GetSENumber() != STATUS_INVALID_HANDLE &&
			ex.GetSENumber() != STATUS_NO_SUCH_DEVICE)
		{
			if (IsDebuggerPresent())
			{
				throw;
			}
			std::auto_ptr<std::basic_string<TCHAR> >
				msg(new std::basic_string<TCHAR>(_T("Error indexing ") + this->_drive + _T(": ") + GetAnyErrorText(ex.GetSENumber())));
			if (PostMessage(this->hWnd, this->wndMessage, ex.GetSENumber(), reinterpret_cast<LPARAM>(msg.get())))
			{ msg.release(); }
		}
	}
	return r;
}
