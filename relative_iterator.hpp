#pragma once

#ifndef RELATIVE_ITERATOR_HPP
#define RELATIVE_ITERATOR_HPP

namespace
{
	template<class T> struct iterator_of { typedef typename T::iterator type; };
	template<class T> struct iterator_of<T const> { typedef typename T::const_iterator type; };

	template<class Container>
	struct relative_iterator :
		public std::iterator<
			typename std::iterator_traits<typename iterator_of<Container>::type>::iterator_category,
			typename std::iterator_traits<typename iterator_of<Container>::type>::value_type,
			typename std::iterator_traits<typename iterator_of<Container>::type>::distance_type>
	{
		typedef typename iterator_of<Container>::type iterator;
		typedef Container container_type;
		typedef relative_iterator This;
		typedef typename std::iterator_traits<iterator>::iterator_category iterator_category;
		typedef typename std::iterator_traits<iterator>::difference_type difference_type;
		typedef typename std::iterator_traits<iterator>::reference reference;
		typedef typename std::iterator_traits<iterator>::pointer pointer;
		typedef typename std::iterator_traits<iterator>::value_type value_type;
		typedef typename container_type::size_type size_type;
		container_type *p;
		size_type offset;
		relative_iterator() : p(), offset() { }
		relative_iterator(container_type &container, size_type const offset = container_type::size_type())
			: p(&container), offset(offset) { }
		iterator   operator ->() const { return this->operator iterator(); }
		value_type &operator *() const { return *this->operator ->(); }
		This &operator ++(   ) { This &me(*this); ++ this->offset; return me; }
		This &operator --(   ) { This &me(*this); -- this->offset; return me; }
		This  operator ++(int) { This  me(*this); ++*this;         return me; }
		This  operator --(int) { This  me(*this); --*this;         return me; }
		bool  operator < (This const &other) const { return this->offset <  other.offset; }
		bool  operator > (This const &other) const { return this->offset >  other.offset; }
		bool  operator <=(This const &other) const { return this->offset <= other.offset; }
		bool  operator >=(This const &other) const { return this->offset >= other.offset; }
		bool  operator ==(This const &other) const { return this->offset == other.offset; }
		bool  operator !=(This const &other) const { return this->offset != other.offset; }
		This &operator +=(distance_type offset) { This &me(*this); me.offset += static_cast<size_type>(offset); return me; }
		This &operator -=(distance_type offset) { This &me(*this); me.offset -= static_cast<size_type>(offset); return me; }
		This  operator + (distance_type offset) const { This me(*this); me.offset += static_cast<size_type>(offset); return me; }
		This  operator - (distance_type offset) const { This me(*this); me.offset -= static_cast<size_type>(offset); return me; }
		difference_type operator - (This const &other) const { return static_cast<difference_type>(this->offset - other.offset); }
		operator iterator() const { iterator it(this->p->begin()); std::advance(it, static_cast<distance_type>(this->offset)); return it; }
	};

	template<class Container>
	typename relative_iterator<Container>
		make_relative_iterator(Container &container, typename Container::size_type const offset = 0)
	{
		return relative_iterator<Container>(container, offset);
	}

	template<class Container>
	typename relative_iterator<Container>
		make_relative_iterator(Container &container, typename Container::const_iterator const i)
	{
		return relative_iterator<Container>(container, std::distance(static_cast<Container::const_iterator>(container.begin()), i));
	}

	template<class Container>
	typename std::pair<relative_iterator<Container>, relative_iterator<Container> >
		make_relative_iterators(Container &container, typename Container::size_type const offset1 = 0, typename Container::size_type const offset2 = 0)
	{
		return std::make_pair(make_relative_iterator(container, offset1), make_relative_iterator(container, offset2));
	}

	template<class Container>
	typename std::pair<relative_iterator<Container>, relative_iterator<Container> >
		make_relative_iterators(Container &container, typename Container::const_iterator const i1, typename Container::const_iterator const i2)
	{
		return std::make_pair(make_relative_iterator(container, i1), make_relative_iterator(container, i2));
	}
}

#endif
