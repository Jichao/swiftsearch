#pragma once

#include <stddef.h>
#include <tchar.h>

#include <string>

#include "ref_counter.hpp"

class NtfsIndex;

class __declspec(novtable) NtfsIndexThread : public virtual ref_counter
{
public:
	virtual ~NtfsIndexThread() { }
	virtual volatile bool &background() = 0;
	virtual boost::shared_ptr<NtfsIndex> index() const = 0;
	virtual std::basic_string<TCHAR> const &drive() const = 0;
	virtual unsigned long progress() const = 0;
	virtual uintptr_t event() const = 0;
	virtual uintptr_t thread() const = 0;
	virtual uintptr_t volume() const = 0;  // MUST return the SAME handle!
	virtual void close() = 0;

	static NtfsIndexThread *create(HWND const hWnd, unsigned int const wndMessageError, std::basic_string<TCHAR> const &drive);
};
