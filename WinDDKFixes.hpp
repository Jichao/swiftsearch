#pragma once

#ifdef __cplusplus
#include <cstdlib>  // needed for _CPPLIB_VER
#include <cstddef>
#if defined(_MSC_VER) && !defined(_CPPLIB_VER) || _CPPLIB_VER < 403

extern "C"
{
#ifdef _WIN64
	__declspec(dllimport) long long (__stdcall *__stdcall GetProcAddress(struct HINSTANCE__* hModule, char const *lpProcName))();
#else
	__declspec(dllimport) int (__stdcall *__stdcall GetProcAddress(struct HINSTANCE__* hModule, char const *lpProcName))();
#endif
	__declspec(dllimport) int __stdcall GetModuleHandleExA(unsigned long dwFlags, char const *lpModuleName, struct HINSTANCE__** phModule);
	__declspec(dllimport) int __stdcall GetModuleHandleExW(unsigned long dwFlags, wchar_t const *lpModuleName, struct HINSTANCE__** phModule);
#if defined(_MSC_VER) && _MSC_VER >= 1400
#ifdef _M_X64
	void *__cdecl _InterlockedCompareExchangePointer(void *volatile *Destination, void *ExChange, void *Comparand);
#	pragma intrinsic(_InterlockedCompareExchangePointer)
#else
	long __cdecl _InterlockedCompareExchange(long volatile *, long, long);
#	pragma intrinsic(_InterlockedCompareExchange)
	static void *__cdecl _InterlockedCompareExchangePointer(void *volatile *Destination, void *ExChange, void *Comparand)
	{
		return (void *)_InterlockedCompareExchange((long volatile *)Destination, (long)ExChange, (long)Comparand);
	}
#endif
#endif
}
#ifndef _C_STD_BEGIN
#	define _C_STD_BEGIN namespace std {
#endif
#ifndef _C_STD_END
#	define _C_STD_END }
#endif
#ifndef _CSTD
#	define _CSTD ::std::
#endif
namespace std { template<class C, class T, class D = ptrdiff_t, class P = T *, class R = void> struct iterator; }
#define iterator iterator_bad
#define inserter inserter_bad
#define insert_iterator insert_iterator_bad
#define iterator_traits iterator_traits_bad
#define reverse_iterator reverse_iterator_bad
#include <utility>  // iterator_traits
template<class T>
struct std::iterator_traits_bad<T *>
{
	typedef std::random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef ptrdiff_t difference_type;
	typedef ptrdiff_t distance_type;
	typedef T *pointer;
	typedef T &reference;
};
#undef reverse_iterator
namespace std { template<class RanIt, class T = typename iterator_traits<RanIt>::value_type, class R = T &, class P = T *, class D = ptrdiff_t> class reverse_iterator; }
#	include <iterator>
#undef iterator_traits
#undef insert_iterator
#undef inserter
#undef iterator

typedef long long _Longlong;
typedef unsigned long long _ULonglong;

#include <math.h>  // ::ceil
#include <limits>

namespace std
{
	using ::intptr_t;
	using ::uintptr_t;
	using ::memcpy;
	using ::memset;
	using ::abort;
	using ::strerror;
	using ::ceil;
	using ::va_list;

	template<class T> T *operator &(T &p) { return reinterpret_cast<T *>(&reinterpret_cast<unsigned char &>(p)); }
	template<class T> T const *operator &(T const &p) { return reinterpret_cast<T const *>(&reinterpret_cast<unsigned char const &>(p)); }

	template<> class numeric_limits<long long>;
	template<> class numeric_limits<unsigned long long>;

	template<class T>
	struct ref_or_void { typedef T &type; };

	template<>
	struct ref_or_void<void> { typedef void type; };

	template<class C, class T, class D, class P, class R>
	struct iterator : public iterator_bad<C, T, D>
	{
		typedef D difference_type;
		typedef P pointer;
		typedef R reference;
	};

	template<class C>
	class insert_iterator : public iterator<output_iterator_tag, void, void>
	{
	public:
		typedef C container_type;
		typedef typename C::value_type value_type;
		insert_iterator(C& _X, typename C::iterator _I) : container(&_X), iter(_I) {}
		insert_iterator<C>& operator=(const value_type& _V) { iter = container->insert(iter, _V); ++iter; return (*this); }
		insert_iterator<C>& operator*() { return (*this); }
		insert_iterator<C>& operator++() { return (*this); }
		insert_iterator<C>& operator++(int) { return (*this); }
	protected:
		C *container;
		typename C::iterator iter;
	};
	
	template<class C, class _XI>
	inline insert_iterator<C> inserter(C& _X, _XI _I)
	{ return (insert_iterator<C>(_X, C::iterator(_I))); }

	template<class It>
	struct iterator_traits //: public iterator_traits_bad<It>
	{
		typedef typename It::iterator_category iterator_category;
		typedef typename It::value_type value_type;
		typedef ptrdiff_t difference_type;
		typedef difference_type distance_type;
		typedef value_type *pointer;
		typedef value_type &reference;
	};

	template<class T>
	struct iterator_traits<T *>
	{
		typedef random_access_iterator_tag iterator_category;
		typedef T value_type;
		typedef ptrdiff_t difference_type;
		typedef ptrdiff_t distance_type;
		typedef T *pointer;
		typedef T &reference;
	};

	template<class C>
	struct iterator_traits<insert_iterator<C> >
	{
		typedef output_iterator_tag iterator_category;
		typedef typename C::value_type value_type;
		typedef void difference_type;
		typedef void distance_type;
		typedef void pointer;
		typedef void reference;
	};

	template<class It>
	inline typename iterator_traits<It>::iterator_category __cdecl _Iter_cat(It const &) { return iterator_traits<It>::iterator_category(); }
}

// std::allocator::rebind doesn't exist!!
#define allocator allocator_bad
#	include <memory>
#undef allocator
namespace std
{
	template<class T>
	class allocator : public allocator_bad<T>
	{
		typedef allocator_bad<T> Base;
	public:
		template<class U> struct rebind { typedef allocator<U> other; };
		allocator() { }
		allocator(Base const &v) : Base(v) { }
		template<class Other>
		allocator(allocator_bad<Other> const &) : Base() { }
		typename Base::pointer allocate(typename Base::size_type n, void const *hint = NULL)
		{ return this->Base::allocate(n, hint); }
	};

	template<class T>
	class allocator<T const> : public allocator<T>
	{
	};
}

// The above re-implementation of std::allocator messes up some warnings...
#pragma warning(push)
#pragma warning(disable: 4251)  // class 'type' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable: 4512)  // assignment operator could not be generated
	namespace std
	{
		template<class RanIt, class T, class R, class P, class D>
		class reverse_iterator : public iterator<typename iterator_traits<RanIt>::iterator_category, T, D, P, R>
		{
		public:
			typedef reverse_iterator<RanIt> This;
			typedef typename iterator_traits<RanIt>::difference_type difference_type;
			typedef typename iterator_traits<RanIt>::pointer pointer;
			typedef typename iterator_traits<RanIt>::reference reference;
			typedef RanIt iterator_type;
			reverse_iterator() { }
			explicit reverse_iterator(RanIt right) : current(right) { }
			template<class Other>
			reverse_iterator(const reverse_iterator<Other>& right) : current(right.base()) { }
			RanIt base() const { return (current); }
			reference operator*() const { RanIt tmp = current; return (*--tmp); }
			pointer operator->() const { return (&**this); }
			This& operator++() { --current; return (*this); }
			This operator++(int) { This tmp = *this; --current; return (tmp); }
			This& operator--() { ++current; return (*this); }
			This operator--(int) { This tmp = *this; ++current; return (tmp); }
			bool operator ==(const reverse_iterator& right) const
			{ return (current == right.base()); }
			bool operator !=(const reverse_iterator& right) const
			{ return (current != right.base()); }
			This& operator+=(difference_type offset) { current -= offset; return (*this); }
			This operator+(difference_type offset) const { return (This(current - offset)); }
			This& operator-=(difference_type offset) { current += offset; return (*this); }
			This operator-(difference_type offset) const { return (This(current + offset)); }
			reference operator[](difference_type offset) const { return (*(*this + offset)); }
			bool operator <(const reverse_iterator& right) const { return (right.base() < current); }
			bool operator >(const reverse_iterator& right) const { return (right.base() > current); }
			bool operator <=(const reverse_iterator& right) const { return (right.base() <= current); }
			bool operator >=(const reverse_iterator& right) const { return (right.base() >= current); }
			difference_type operator -(const reverse_iterator& right) const { return (right.base() - current); }
		protected:
			RanIt current;
		};
		template<class RanIt, class Diff> inline reverse_iterator<RanIt> operator+(Diff offset, const reverse_iterator<RanIt>& right) { return (right + offset); }
		//template<class RanIt1, class RanIt2> inline typename reverse_iterator<RanIt1>::difference_type operator-(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (left - right); }
		//template<class RanIt1, class RanIt2> inline bool operator==(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (left == right); }
		//template<class RanIt1, class RanIt2> inline bool operator!=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (!(left.operator ==(right))); }
		//template<class RanIt1, class RanIt2> inline bool operator<(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (left < right); }
		//template<class RanIt1, class RanIt2> inline bool operator>(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (right < left); }
		//template<class RanIt1, class RanIt2> inline bool operator<=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (!(right < left)); }
		//template<class RanIt1, class RanIt2> inline bool operator>=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right) { return (!(left < right)); }
	}
	// Fixes for 'vector' -- boost::ptr_vector chokes on the old implementation!
#	include <vector>
	namespace std
	{
		struct _PVOID
		{
			void *p;
			_PVOID(void *const &p = 0) : p(p) { }
			template<class T> operator T *&() { return reinterpret_cast<T *&>(p); }
			template<class T> operator T *const &() const { return reinterpret_cast<T *const &>(p); }
		};
	}
	template<>
	class std::vector<void *, std::allocator<void *> > : public std::vector<void *, std::allocator<_PVOID> >
	{
	public:
		using std::vector<void *, std::allocator<_PVOID> >::insert;
		template<class It>
		void insert(iterator it, It begin, It end) { std::copy(begin, end, std::inserter(*this, it)); }
	};

#if 0
	template<>
	struct std::iterator_traits<std::vector<_Bool,_Bool_allocator>::_It>
	{
		typedef std::random_access_iterator_tag iterator_category;
		typedef unsigned int value_type;
		typedef ptrdiff_t difference_type;
		typedef ptrdiff_t distance_type;
		typedef unsigned int *pointer;
		typedef unsigned int &reference;
	};
#endif


#if !defined(_NATIVE_WCHAR_T_DEFINED) && (defined(DDK_CTYPE_WCHAR_FIX) && DDK_CTYPE_WCHAR_FIX)
#ifdef _DLL
	namespace std
	{
		template <class> class ctype;
		template <> class ctype<wchar_t>;
	}
#undef _DLL
#include <xlocale>
#define _DLL
	namespace std
	{
		template <>
		class ctype<wchar_t> : public ctype_base {
		public:
			typedef wchar_t _E;
			typedef _E char_type;
			bool is(mask _M, _E _C) const
				{return ((_Ctype._Table[(wchar_t)_C] & _M) != 0); }
			const _E *is(const _E *_F, const _E *_L, mask *_V) const
				{for (; _F != _L; ++_F, ++_V)
					*_V = _Ctype._Table[(wchar_t)*_F];
				return (_F); }
			const _E *scan_is(mask _M, const _E *_F,
				const _E *_L) const
				{for (; _F != _L && !is(_M, *_F); ++_F)
					;
				return (_F); }
			const _E *scan_not(mask _M, const _E *_F,
				const _E *_L) const
				{for (; _F != _L && is(_M, *_F); ++_F)
					;
				return (_F); }
			_E tolower(_E _C) const
				{return (do_tolower(_C)); }
			const _E *tolower(_E *_F, const _E *_L) const
				{return (do_tolower(_F, _L)); }
			_E toupper(_E _C) const
				{return (do_toupper(_C)); }
			const _E *toupper(_E *_F, const _E *_L) const
				{return (do_toupper(_F, _L)); }
			_E widen(wchar_t _X) const
				{return (_X); }
			const _E *widen(const wchar_t *_F, const wchar_t *_L, _E *_V) const
				{memcpy(_V, _F, _L - _F);
				return (_L); }
			_E narrow(_E _C, wchar_t _D = '\0') const
				{(_D); return (_C); }
			const _E *narrow(const _E *_F, const _E *_L, wchar_t _D,
				wchar_t *_V) const
				{(_D);memcpy(_V, _F, _L - _F);
				return (_L); }
			static locale::id id;
			explicit ctype(const mask *_Tab = 0, bool _Df = false,
				size_t _R = 0)
				: ctype_base(_R)
				{_Lockit Lk;
				_Init(_Locinfo());
				if (_Ctype._Delfl)
					free((void *)_Ctype._Table), _Ctype._Delfl = false;
				if (_Tab == 0)
					_Ctype._Table = _Cltab;
				else
					_Ctype._Table = _Tab, _Ctype._Delfl = _Df; }
			ctype(const _Locinfo& _Lobj, size_t _R = 0)
				: ctype_base(_R) {_Init(_Lobj); }
			static size_t __cdecl _Getcat()
				{return (_LC_CTYPE); }
			static const size_t table_size;
		_PROTECTED:
			virtual ~ctype()
				{if (_Ctype._Delfl)
					free((void *)_Ctype._Table); }
		protected:
			static void __cdecl _Term(void)
				{free((void *)_Cltab); }
			void _Init(const _Locinfo& _Lobj)
				{_Lockit Lk;
				_Ctype = _Lobj._Getctype();
				if (_Cltab == 0)
					{_Cltab = _Ctype._Table;
					atexit(_Term);
					_Ctype._Delfl = false; }}
			virtual _E do_tolower(_E _C) const
				{return (_E)(_Tolower((wchar_t)_C, &_Ctype)); }
			virtual const _E *do_tolower(_E *_F, const _E *_L) const
				{for (; _F != _L; ++_F)
					*_F = (_E)_Tolower(*_F, &_Ctype);
				return ((const _E *)_F); }
			virtual _E do_toupper(_E _C) const
				{return ((_E)_Toupper((wchar_t)_C, &_Ctype)); }
			virtual const _E *do_toupper(_E *_F, const _E *_L) const
				{for (; _F != _L; ++_F)
					*_F = (_E)_Toupper(*_F, &_Ctype);
				return ((const _E *)_F); }
			const mask *table() const _THROW0()
				{return (_Ctype._Table); }
			static const mask * __cdecl classic_table() _THROW0()
				{_Lockit Lk;
				if (_Cltab == 0)
					locale::classic();      // force locale::_Init() call
				return (_Cltab); }
		private:
			_Locinfo::_Ctypevec _Ctype;
			static const mask *_Cltab;
		};
		namespace
		{
			struct HINSTANCE__* get_msvcprt_handle()
			{
				static struct HINSTANCE__ *volatile g_hMSCRP60 = NULL;
				if (!g_hMSCRP60)
				{
					struct HINSTANCE__ *hMSCRP60 = NULL;
#if defined(_UNICODE) || defined(UNICODE)
					GetModuleHandleExW(0x4 | 0x2, reinterpret_cast<wchar_t const *>(&ctype<char>::id), &hMSCRP60);
#else
					GetModuleHandleExA(0x4 | 0x2, reinterpret_cast<char const *>(&ctype<char>::id), &hMSCRP60);
#endif
					_InterlockedCompareExchangePointer(&reinterpret_cast<void *volatile &>(g_hMSCRP60), hMSCRP60, NULL);
				}
				return g_hMSCRP60;
			}
		}
		template<> inline
			const ctype<wchar_t>& __cdecl use_facet<ctype<wchar_t> >(const locale& _L, const ctype<wchar_t> *,
				bool _Cfacet)
			{static const locale::facet *_Psave = 0;
			_Lockit _Lk;
			static locale::id *volatile g_ctype_wchar_t_id = NULL;
			if (!g_ctype_wchar_t_id)
			{ _InterlockedCompareExchangePointer(&reinterpret_cast<void *volatile &>(g_ctype_wchar_t_id), reinterpret_cast<locale::id *>(GetProcAddress(get_msvcprt_handle(), "?id@?$ctype@G@std@@2V0locale@2@A")), NULL); }
			size_t _Id = *g_ctype_wchar_t_id;
			const locale::facet *_Pf = _L._Getfacet(_Id, true);
			if (_Pf != 0)
				;
			else if (!_Cfacet || !_L._Iscloc())
				_THROW(bad_cast, "missing locale facet");
			else if (_Psave == 0)
				_Pf = _Psave = _Tidyfac<ctype<wchar_t>>::_Save(new ctype<wchar_t>);
			else
				_Pf = _Psave;
			return (*(const ctype<wchar_t> *)_Pf); }
	}
#endif
#endif

#	include <sstream>  // get rid of warnings
#pragma warning(pop)

#include <locale>
using std::codecvt;

#pragma warning(push)
#pragma warning(disable: 4127)  // conditional expression is constant
#include <fstream>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 4512)  // assignment operator could not be generated
#include <set>
#pragma warning(pop)

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max
#include <valarray>
#pragma pop_macro("max")
#pragma pop_macro("min")

#pragma push_macro("_set_se_translator")
extern "C" __inline _se_translator_function __cdecl __set_se_translator(_se_translator_function f)
{
	_se_translator_function (__cdecl *p_set_se_translator)(_se_translator_function f) = &_set_se_translator;
	return p_set_se_translator(f);
}
#define _set_se_translator __set_se_translator
typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS;
typedef unsigned int UINT;
#include <ProvExce.h>
#pragma pop_macro("_set_se_translator")

#endif

#endif
