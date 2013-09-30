#pragma once

#include <stddef.h>
#include <tchar.h>

#include <string>

#include <boost/smart_ptr/shared_ptr.hpp>

#include "ref_counter.hpp"

class NtfsIndex;

class NtfsIndexThread : public virtual ref_counter
{
	std::basic_string<TCHAR> _drive;
	unsigned long volatile _progress;
	unsigned long volatile _speed;
	bool volatile _background;
	boost::shared_ptr<NtfsIndex> _index;
	winnt::NtObject _thread, _event;
	winnt::NtFile _volume;
	HWND hWnd;
	unsigned int wndMessage;

	NtfsIndexThread &operator =(NtfsIndexThread const &);

public:
	NtfsIndexThread(HWND const hWnd, unsigned int wndMessage, std::basic_string<TCHAR> const &drive);
	~NtfsIndexThread();
	volatile bool &background() { return this->_background; }
	volatile bool const &background() const { return this->_background; }
	void close();
	std::basic_string<TCHAR> const &drive() const { return this->_drive; }
	unsigned long progress() const { return this->_progress; }
	unsigned long speed() const { return this->_speed; }
	boost::shared_ptr<NtfsIndex> delete_index();
	uintptr_t volume() const;
	uintptr_t thread() const;
	uintptr_t event() const;
	bool has_index() const;
	boost::shared_ptr<NtfsIndex> index() const;

	unsigned int operator()();
};
