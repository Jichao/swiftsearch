#pragma once

#include <stddef.h>
#include <tchar.h>

#include <string>

#include <boost/shared_ptr.hpp>

class NtfsIndex;

class __declspec(novtable) NtfsIndexThread
{
public:
	virtual ~NtfsIndexThread() { }
	virtual volatile bool &background() = 0;
	virtual volatile bool &cached() = 0;
	virtual boost::shared_ptr<NtfsIndex> index() const = 0;
	virtual std::basic_string<TCHAR> const &drive() const = 0;
	virtual unsigned long progress() const = 0;
	virtual uintptr_t event() const = 0;
	virtual uintptr_t thread() const = 0;

	static NtfsIndexThread *create(std::basic_string<TCHAR> const &drive);
};
