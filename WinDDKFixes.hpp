#pragma once

#ifdef __cplusplus
#include <cstdlib>  // needed for _CPPLIB_VER
#include <cstddef>
#if defined(_MSC_VER) && !defined(_CPPLIB_VER) || _CPPLIB_VER < 403

#	ifndef _C_STD_BEGIN
#		define _C_STD_BEGIN namespace std {
#	endif
#	ifndef _C_STD_END
#		define _C_STD_END }
#	endif
#	ifndef _CSTD
#		define _CSTD ::std::
#	endif
	namespace std { template<class C, class T, class D = ptrdiff_t, class P = T *, class R = void> struct iterator; }
#	define iterator iterator_bad
#	define inserter inserter_bad
#	define insert_iterator insert_iterator_bad
#	define iterator_traits iterator_traits_bad
#		include <iterator>
#		include <utility>  // iterator_traits
#	undef iterator_traits
#	undef insert_iterator
#	undef inserter
#	undef iterator

typedef long long _Longlong;
typedef unsigned long long _ULonglong;

#include <math.h>  // ::ceil

namespace std
{
	using ::intptr_t;
	using ::uintptr_t;
	using ::memset;
	using ::abort;
	using ::strerror;
	using ::ceil;
	using ::va_list;

	template<class T>
	struct ref_or_void { typedef T &type; };

	template<>
	struct ref_or_void<void> { typedef void type; };

	template<class C, class T, class D, class P, class R>
	struct iterator : public iterator_bad<C, T, D>
	{
		typedef D difference_type;
		typedef P pointer;
		typedef typename ref_or_void<R>::type reference;
	};

	template<class _C>
	class insert_iterator : public iterator<output_iterator_tag, void, void>
	{
	public:
		typedef _C container_type;
		typedef typename _C::value_type value_type;
		insert_iterator(_C& _X, typename _C::iterator _I) : container(&_X), iter(_I) {}
		insert_iterator<_C>& operator=(const value_type& _V) { iter = container->insert(iter, _V); ++iter; return (*this); }
		insert_iterator<_C>& operator*() { return (*this); }
		insert_iterator<_C>& operator++() { return (*this); }
		insert_iterator<_C>& operator++(int) { return (*this); }
	protected:
		_C *container;
		typename _C::iterator iter;
	};
	
	template<class _C, class _XI>
	inline insert_iterator<_C> inserter(_C& _X, _XI _I)
	{ return (insert_iterator<_C>(_X, _C::iterator(_I))); }

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

		template<>
		class vector<void *, std::allocator<void *> > : public vector<void *, std::allocator<_PVOID> >
		{
		public:
			using vector<void *, std::allocator<_PVOID> >::insert;
			template<class It>
			void insert(iterator it, It begin, It end) { copy(begin, end, inserter(*this, it)); }
		};

		template<>
		struct iterator_traits<vector<_Bool,_Bool_allocator>::_It>
		{
			typedef random_access_iterator_tag iterator_category;
			typedef unsigned int value_type;
			typedef ptrdiff_t difference_type;
			typedef ptrdiff_t distance_type;
			typedef unsigned int *pointer;
			typedef unsigned int &reference;
		};

	#if 0
		template<class RanIt>
		class reverse_iterator : public iterator<
			typename iterator_traits<RanIt>::iterator_category,
			typename iterator_traits<RanIt>::value_type,
			typename iterator_traits<RanIt>::difference_type,
			typename iterator_traits<RanIt>::pointer,
			typename iterator_traits<RanIt>::reference>
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
			template<class Other>
			bool operator ==(const reverse_iterator<Other>& right) const
			{ return (current == right.base()); }
			This& operator+=(difference_type offset) { current -= offset; return (*this); }
			This operator+(difference_type offset) const { return (This(current - offset)); }
			This& operator-=(difference_type offset) { current += offset; return (*this); }
			This operator-(difference_type offset) const { return (This(current + offset)); }
			reference operator[](difference_type offset) const { return (*(*this + offset)); }
			template<class Other>
			bool operator <(const reverse_iterator<Other>& right) const
			{ return (right.base() < current); }
			template<class Other>
			difference_type operator -(const reverse_iterator<Other>& right) const
			{ return (right.base() - current); }
		protected:
			RanIt current;
		};
		template<class RanIt, class Diff>
		inline reverse_iterator<RanIt> operator+(Diff offset, const reverse_iterator<RanIt>& right)
		{ return (right + offset); }
		template<class RanIt1, class RanIt2>
		inline typename reverse_iterator<RanIt1>::difference_type
			operator-(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (left - right); }
		template<class RanIt1, class RanIt2>
		inline bool operator==(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (left == right); }
		template<class RanIt1, class RanIt2>
		inline bool operator!=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (!(left == right)); }
		template<class RanIt1, class RanIt2>
		inline bool operator<(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (left < right); }
		template<class RanIt1, class RanIt2>
		inline bool operator>(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (right < left); }
		template<class RanIt1, class RanIt2>
		inline bool operator<=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (!(right < left)); }
		template<class RanIt1, class RanIt2>
		inline bool operator>=(const reverse_iterator<RanIt1>& left, const reverse_iterator<RanIt2>& right)
		{ return (!(left < right)); }
	#endif
	}

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