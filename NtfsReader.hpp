#pragma once

namespace winnt { class NtFile; }

class __declspec(novtable) NtfsReader
{
public:
	virtual ~NtfsReader() { }
	virtual size_t size() const = 0;
	virtual bool set_cached(bool value) = 0;
	virtual size_t find_next(size_t const i) const = 0;
	virtual void const *operator[](size_t const i) const = 0;
	static NtfsReader *create(winnt::NtFile const &volume, bool cached);
};