#pragma once

class Threads : public std::vector<uintptr_t> {
	Threads(Threads const &) { }
	Threads &operator =(Threads const &)
	{
		return *this;
	}
public:
	Threads() { }
	~Threads()
	{
		while (!this->empty()) {
			size_t n = std::min(this->size(), static_cast<size_t>(MAXIMUM_WAIT_OBJECTS));
			WaitForMultipleObjects(static_cast<unsigned long>(n),
				reinterpret_cast<void *const *>(&*this->begin() + this->size() - n), TRUE, INFINITE);
			while (n) {
				this->pop_back();
				--n;
			}
		}
	}
};

template<class Derived>
class RefCounted
{
	mutable boost::atomic<unsigned int> refs;
	friend void intrusive_ptr_add_ref(RefCounted const volatile *p) { p->refs.fetch_add(1, boost::memory_order_acq_rel); }
	friend void intrusive_ptr_release(RefCounted const volatile *p)
	{
		if (p->refs.fetch_sub(1, boost::memory_order_acq_rel) - 1 == 0)
		{
			delete static_cast<Derived const volatile *>(p);
		}
	}

protected:
	RefCounted() : refs(0) { }
	RefCounted(RefCounted const &) : refs(0) { }
	~RefCounted() { }
	RefCounted &operator =(RefCounted const &) { }
	void swap(RefCounted &) { }
};

namespace std
{
	typedef basic_string<TCHAR> tstring;
	template<class> struct is_scalar;
#if defined(_CPPLIB_VER) && 600 <= _CPPLIB_VER
#ifdef _XMEMORY_
	template<class T1, class T2> struct is_scalar<std::pair<T1, T2> > :
		integral_constant<bool, is_pod<T1>::value && is_pod<T2>::value>{};
	template<class T1, class T2, class _Diff, class _Valty>
	inline void _Uninit_def_fill_n(std::pair<T1, T2> *_First, _Diff _Count,
		_Wrap_alloc<allocator<std::pair<T1, T2> > >&, _Valty *, _Scalar_ptr_iterator_tag)
	{
		_Fill_n(_First, _Count, _Valty());
	}
#endif
#else
	template<class T> struct is_pod {
		static bool const value = __is_pod(T);
	};
#endif
}

class mutex
{
	void init()
	{
#if defined(_WIN32)
		InitializeCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_init_lock(&p);
#endif
	}
	void term()
	{
#if defined(_WIN32)
		DeleteCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_destroy_lock(&p);
#endif
	}
public:
	mutex &operator =(mutex const &)
	{
		return *this;
	}
#if defined(_WIN32)
	CRITICAL_SECTION p;
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
	omp_lock_t p;
#elif defined(BOOST_THREAD_MUTEX_HPP)
	boost::mutex m;
#else
	std::mutex m;
#endif
	mutex(mutex const &)
	{
		this->init();
	}
	mutex()
	{
		this->init();
	}
	~mutex()
	{
		this->term();
	}
	void lock()
	{
#if defined(_WIN32)
		EnterCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_set_lock(&p);
#else
		return m.lock();
#endif
	}
	void unlock()
	{
#if defined(_WIN32)
		LeaveCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_unset_lock(&p);
#else
		return m.unlock();
#endif
	}
	bool try_lock()
	{
#if defined(_WIN32)
		return !!TryEnterCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		return !!omp_test_lock(&p);
#else
		return m.try_lock();
#endif
	}
};
template<class Mutex>
struct lock_guard {
	typedef Mutex mutex_type;
	mutex_type *p;
	~lock_guard()
	{
		if (p) {
			p->unlock();
		}
	}
	lock_guard() : p() { }
	explicit lock_guard(mutex_type *const mutex,
		bool const already_locked = false) : p(mutex)
	{
		if (p && !already_locked) {
			p->lock();
		}
	}
	explicit lock_guard(mutex_type &mutex, bool const already_locked = false) : p(&mutex)
	{
		if (!already_locked) {
			p->lock();
		}
	}
	lock_guard(lock_guard const &other) : p(other.p)
	{
		if (p) {
			p->lock();
		}
	}
	lock_guard &operator =(lock_guard other)
	{
		return this->swap(other), *this;
	}
	void swap(lock_guard &other)
	{
		using std::swap;
		swap(this->p, other.p);
	}
	friend void swap(lock_guard &a, lock_guard &b)
	{
		return a.swap(b);
	}
};

struct tchar_ci_traits : public std::char_traits<TCHAR>
{
	typedef std::char_traits<TCHAR> Base;
	static bool eq(TCHAR c1, TCHAR c2)
	{
		return c1 < SCHAR_MAX
			? c1 == c2 ||
			_T('A') <= c1 && c1 <= _T('Z') && c1 - c2 == _T('A') - _T('a') ||
			_T('A') <= c2 && c2 <= _T('Z') && c2 - c1 == _T('A') - _T('a')
			: _totupper(c1) == _totupper(c2);
	}
	static bool ne(TCHAR c1, TCHAR c2)
	{
		return !eq(c1, c2);
	}
	static bool lt(TCHAR c1, TCHAR c2)
	{
		return _totupper(c1) < _totupper(c2);
	}
	static int compare(TCHAR const *s1, TCHAR const *s2, size_t n)
	{
		while (n-- != 0) {
			if (lt(*s1, *s2)) {
				return -1;
			}
			if (lt(*s2, *s1)) {
				return +1;
			}
			++s1;
			++s2;
		}
		return 0;
	}
	static TCHAR const *find(TCHAR const *s, size_t n, TCHAR a)
	{
		while (n > 0 && ne(*s, a)) {
			++s;
			--n;
		}
		return s;
	}
};

template<class It1, class It2, class Tr>
inline bool wildcard(It1 patBegin, It1 const patEnd, It2 strBegin, It2 const strEnd,
	Tr const &tr = std::char_traits<typename std::iterator_traits<It2>::value_type>())
{
	(void)tr;
	if (patBegin == patEnd) {
		return strBegin == strEnd;
	}
	//http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
	It2 s(strBegin);
	It1 p(patBegin);
	bool star = false;

loop:
	for (s = strBegin, p = patBegin; s != strEnd && p != patEnd; ++s, ++p) {
		if (tr.eq(*p, _T('*'))) {
			star = true;
			strBegin = s, patBegin = p;
			if (++patBegin == patEnd) {
				return true;
			}
			goto loop;
		}
		else if (!tr.eq(*p, _T('?')) && !tr.eq(*s, *p)) {
			if (!star) {
				return false;
			}
			strBegin++;
			goto loop;
		}
	}
	while (p != patEnd && tr.eq(*p, _T('*'))) {
		++p;
	}
	return p == patEnd && s == strEnd;
}

class Handle {
	static bool valid(void *const value);
public:
	void *value;
	Handle();
	explicit Handle(void *const value);
	Handle(Handle const &other);
	~Handle();
	Handle &operator =(Handle other);
	operator void *() const;
	void swap(Handle &other);
	friend void swap(Handle &a, Handle &b)
	{
		return a.swap(b);
	}
};

class Overlapped : public OVERLAPPED, public RefCounted<Overlapped> {
	Overlapped(Overlapped const &);
	Overlapped &operator =(Overlapped const &);
public:
	virtual ~Overlapped() { }
	Overlapped() : OVERLAPPED() { }
	virtual int /* > 0 if re-queue requested, = 0 if no re-queue but no destruction, < 0 if destruction requested */
		operator()(size_t const size, uintptr_t const /*key*/) = 0;
	long long offset() const
	{
		return (static_cast<long long>(this->OVERLAPPED::OffsetHigh) << (CHAR_BIT * sizeof(
			this->OVERLAPPED::Offset))) | this->OVERLAPPED::Offset;
	}
	void offset(long long const value)
	{
		this->OVERLAPPED::Offset = static_cast<unsigned long>(value);
		this->OVERLAPPED::OffsetHigh = static_cast<unsigned long>(value >> (CHAR_BIT * sizeof(
			this->OVERLAPPED::Offset)));
	}
};


class CoInit {
	CoInit(CoInit const &) : hr(S_FALSE) { }
	CoInit &operator =(CoInit const &) { }
public:
	HRESULT hr;
	CoInit(bool initialize = true) : hr(initialize ? CoInitialize(NULL) : S_FALSE) { }
	~CoInit()
	{
		if (this->hr == S_OK) {
			CoUninitialize();
		}
	}
};


