#include "stdafx.h"
#include "targetver.h"

#include "NtfsIndexThread.hpp"

#include <process.h>

#include <algorithm>
#include <iterator>
#include <string>

#include <boost/smart_ptr/shared_ptr.hpp>

#include "NtfsIndex.hpp"

#include "NtObjectMini.hpp"

class NtfsIndexThreadImpl : public NtfsIndexThread
{
	std::basic_string<TCHAR> _drive;
	unsigned long volatile _progress;
	bool volatile _cached;
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
		: _drive(drive), _event(CreateEvent(NULL, TRUE, TRUE, NULL)), _cached(true), _background(true)
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

	volatile bool &cached() { return this->_cached; }

	volatile bool const &cached() const { return this->_cached; }

	uintptr_t thread() const { return reinterpret_cast<uintptr_t>(this->_thread.get()); }

	std::basic_string<TCHAR> const &drive() const { return this->_drive; }

	unsigned long progress() const { return this->_progress; }

	uintptr_t event() const { return reinterpret_cast<uintptr_t>(this->_event.get()); }

	NtfsIndex *index() const { return this->_index.get(); }

	unsigned int operator()()
	{
		Sleep((37 * GetCurrentThreadId()) % 100);
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
		unsigned int r = 0;
		winnt::NtFile volume;
		std::basic_string<TCHAR> ntPath;
		try
		{
			ntPath = winnt::NtFile::RtlDosPathNameToNtPathName((this->_drive + _T("\\$Volume")).c_str());
			volume = winnt::NtFile::NtOpenFile(
				ntPath,
				winnt::Access::QueryAttributes | winnt::Access::Read | winnt::Access::Synchronize,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
		}
		catch (CStructured_Exception &ex) { r = ex.GetSENumber(); }
		if (volume.get())
		{
			std::basic_string<TCHAR> name = volume.GetWin32Path();
			std::string s = "Worker thread for drive: ";
			std::copy(name.begin(), name.end(), std::inserter(s, s.end()));
			SetThreadName(s.c_str());
			boost::shared_ptr<NtfsIndex> old;
			boost::atomic_compare_exchange(
				&this->_index,
				&old,
				boost::shared_ptr<NtfsIndex>(NtfsIndex::create(volume, this->_event, &this->_progress, &this->_cached, &this->_background)));
		}
		return r;
	}
};

NtfsIndexThread *NtfsIndexThread::create(std::basic_string<TCHAR> const &drive)
{
	return new NtfsIndexThreadImpl(drive);
}