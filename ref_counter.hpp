#pragma once

extern "C"
{
	long __cdecl _InterlockedIncrement(long volatile *Addend);
#pragma intrinsic(_InterlockedIncrement)
	long __cdecl _InterlockedDecrement(long volatile *Addend);
#pragma intrinsic(_InterlockedDecrement)
}

class ref_counter
{
	mutable long volatile refcount;
protected:
	ref_counter() : refcount(0) { }
	ref_counter(ref_counter const &) : refcount(0) { }
	ref_counter &operator =(ref_counter const &) { return *this; }
	void swap(ref_counter &) { }  // noexcept
public:
	virtual ~ref_counter() { }
	friend void intrusive_ptr_add_ref(ref_counter *p) { _InterlockedIncrement(&p->refcount); }
	friend void intrusive_ptr_release(ref_counter *p) { if (_InterlockedDecrement(&p->refcount) == 0) { delete p; } }
};
