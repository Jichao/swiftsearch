#include "stdafx.h"
#include "targetver.h"

#include "NtfsIndexThread.hpp"

#include <process.h>

#include <algorithm>
#include <iterator>
#include <string>

#include <boost/range/algorithm/copy.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include "NtfsIndex.hpp"

#include "NtObjectMini.hpp"

TCHAR const *GetAnyErrorText(unsigned long errorCode, va_list* pArgList = NULL);


class NtfsIndexThreadImpl : public NtfsIndexThread
{
	std::basic_string<TCHAR> _drive;
	unsigned long volatile _progress;
	bool volatile _background;
	boost::shared_ptr<NtfsIndex> _index;
	winnt::NtThread _thread;
	winnt::NtEvent _event;

	NtfsIndexThreadImpl &operator =(NtfsIndexThreadImpl const &);

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

public:
	NtfsIndexThreadImpl(std::basic_string<TCHAR> const drive)
		: _drive(drive), _event(CreateEvent(NULL, TRUE, TRUE, NULL)), _background(true)
	{
		struct Invoker
		{
			static unsigned int __stdcall invoke(void *me)
			{ return (*static_cast<NtfsIndexThreadImpl *>(me))(); }
		};
		unsigned int tid;
		*this->_thread.receive() = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &Invoker::invoke, this, CREATE_SUSPENDED, &tid));
		ResumeThread(this->_thread.get());
	}

	~NtfsIndexThreadImpl()
	{
		InterlockedExchange(&reinterpret_cast<long volatile &>(this->_progress), static_cast<long>((std::numeric_limits<unsigned long>::max)()));
		this->_thread.NtWaitForSingleObject();
	}

	volatile bool &background() { return this->_background; }

	volatile bool const &background() const { return this->_background; }

	uintptr_t thread() const { return reinterpret_cast<uintptr_t>(this->_thread.get()); }

	std::basic_string<TCHAR> const &drive() const { return this->_drive; }

	unsigned long progress() const { return this->_progress; }

	uintptr_t event() const { return reinterpret_cast<uintptr_t>(this->_event.get()); }

	boost::shared_ptr<NtfsIndex> index() const { return this->_index; }

	unsigned int operator()()
	{
		Sleep((37 * GetCurrentThreadId()) % 100);
		unsigned int r = 0;
		winnt::NtFile volume;
		try
		{
			try
			{
				std::basic_string<TCHAR> ntPath = winnt::NtFile::RtlDosPathNameToNtPathName(this->_drive.c_str());
				while (!ntPath.empty() && ntPath[ntPath.size() - 1] == _T('\\')) { ntPath.resize(ntPath.size() - 1); }
				ntPath += _T("\\$Volume");
				volume = winnt::NtFile::NtOpenFile(
					ntPath,
					winnt::Access::QueryAttributes | winnt::Access::Read | winnt::Access::Synchronize,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
			}
			catch (CStructured_Exception &ex) { r = ex.GetSENumber(); }
			if (volume.get())
			{
				if (IsDebuggerPresent())
				{
					std::string s = "Worker thread for drive: ";
					boost::copy(this->_drive, std::inserter(s, s.end()));
					SetThreadName(s.c_str());
				}
				for (; ; )
				{
					FILETIME creationTime = { }, exitTime = { }, kernelTime1 = { }, kernelTime2 = { }, userTime2 = { }, userTime1 = { };
					GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime1, &userTime1);
					
					boost::atomic_exchange(&this->_index, boost::shared_ptr<NtfsIndex>(NtfsIndex::create(volume, this->_drive, this->_event, &this->_progress, &this->_background)));

					if (!GetThreadTimes(GetCurrentThread(), &creationTime, &exitTime, &kernelTime2, &userTime2)) { break; }
					LARGE_INTEGER q1 = { }, q2 = { };
					q1.LowPart = userTime1.dwLowDateTime + kernelTime1.dwLowDateTime;
					q1.HighPart = userTime1.dwHighDateTime + kernelTime1.dwHighDateTime;
					q2.LowPart = userTime2.dwLowDateTime + kernelTime2.dwLowDateTime;
					q2.HighPart = userTime2.dwHighDateTime + kernelTime2.dwHighDateTime;
					unsigned long const sleepInterval = 100;  // The program can't exit while sleeping! So keep this value low.
					using std::max;
					using std::min;
					for (long long nMillisToSleep = min(60 * 1000, max(15 * 1000, 10 * (q2.QuadPart - q1.QuadPart) / 10000)); nMillisToSleep > 0; nMillisToSleep -= sleepInterval)
					{
						if (this->_progress == NtfsIndex::PROGRESS_CANCEL_REQUESTED)
						{ throw CStructured_Exception(ERROR_CANCELLED, NULL); }
						Sleep(sleepInterval);
					}
				}
			}
		}
		catch (CStructured_Exception &ex)
		{
			if (ex.GetSENumber() != ERROR_CANCELLED)
			{
				if (!IsDebuggerPresent())
				{
					WTL::AtlMessageBox(NULL, GetAnyErrorText(ex.GetSENumber()), (_T("Error indexing ") + this->_drive).c_str(), MB_ICONERROR | MB_OK | MB_APPLMODAL);
				}
				else { throw; }
			}
		}
		return r;
	}
};

NtfsIndexThread *NtfsIndexThread::create(std::basic_string<TCHAR> const &drive)
{
	return new NtfsIndexThreadImpl(drive);
}