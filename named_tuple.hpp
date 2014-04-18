#define DECLARE_NAMED_1TUPLE(This, T1, t1)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(1);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	T1 t1;  \
	explicit This() : t1() { }  \
	explicit This(T1 const &t1)  \
		: t1(t1) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1> const &t)  \
		: t1(std::get<1 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1> >::type const &get() const;  \
	operator std::tuple<T1>() const { return std::tuple<T1>(t1); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	void swap(std::tuple<T1> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_2TUPLE(This, T1, t1, T2, t2)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(2);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	T1 t1; T2 t2;  \
	explicit This() : t1(), t2() { }  \
	explicit This(T1 const &t1, T2 const &t2)  \
		: t1(t1), t2(t2) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2> >::type const &get() const;  \
	operator std::tuple<T1, T2>() const { return std::tuple<T1, T2>(t1, t2); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	void swap(std::tuple<T1, T2> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_3TUPLE(This, T1, t1, T2, t2, T3, t3)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(3);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	T1 t1; T2 t2; T3 t3;  \
	explicit This() : t1(), t2(), t3() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3)  \
		: t1(t1), t2(t2), t3(t3) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3>() const { return std::tuple<T1, T2, T3>(t1, t2, t3); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	void swap(std::tuple<T1, T2, T3> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_4TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(4);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4;  \
	explicit This() : t1(), t2(), t3(), t4() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4)  \
		: t1(t1), t2(t2), t3(t3), t4(t4) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4>() const { return std::tuple<T1, T2, T3, T4>(t1, t2, t3, t4); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	void swap(std::tuple<T1, T2, T3, T4> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_5TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(5);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5;  \
	explicit This() : t1(), t2(), t3(), t4(), t5() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5>() const { return std::tuple<T1, T2, T3, T4, T5>(t1, t2, t3, t4, t5); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_6TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(6);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6>() const { return std::tuple<T1, T2, T3, T4, T5, T6>(t1, t2, t3, t4, t5, t6); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_7TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(7);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7>(t1, t2, t3, t4, t5, t6, t7); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_8TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(8);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8>(t1, t2, t3, t4, t5, t6, t7, t8); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_9TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(9);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9>(t1, t2, t3, t4, t5, t6, t7, t8, t9); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_10TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(10);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_11TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(11);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_12TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(12);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_13TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(13);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_14TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(14);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_15TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(15);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_16TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15, T16, t16)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(16);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	template<> struct element_type<16 - 1> { typedef T16 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15; T16 t16;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15(), t16() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15, T16 const &t16)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15), t16(t16) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; r = r && this->t16 == other.t16; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; r = r || this->t16 != other.t16; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; r = r && this->t16 <= other.t16; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; r = r && this->t16 >= other.t16; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		if (!r && e) { r = this->t16 < other.t16; e = !r && !(other.t16 < this->t16); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		if (!r && e) { r = this->t16 > other.t16; e = !r && !(other.t16 > this->t16); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); swap(this->t16, other.t16); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)), t16(std::get<16 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	template<> T16 &get<16 - 1>() { return this->t16; }  \
	template<> T16 const &get<16 - 1>() const { return this->t16; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
		swap(this->t16, get<16 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_17TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15, T16, t16, T17, t17)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(17);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	template<> struct element_type<16 - 1> { typedef T16 type; };  \
	template<> struct element_type<17 - 1> { typedef T17 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15; T16 t16; T17 t17;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15(), t16(), t17() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15, T16 const &t16, T17 const &t17)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15), t16(t16), t17(t17) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; r = r && this->t16 == other.t16; r = r && this->t17 == other.t17; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; r = r || this->t16 != other.t16; r = r || this->t17 != other.t17; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; r = r && this->t16 <= other.t16; r = r && this->t17 <= other.t17; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; r = r && this->t16 >= other.t16; r = r && this->t17 >= other.t17; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		if (!r && e) { r = this->t16 < other.t16; e = !r && !(other.t16 < this->t16); }  \
		if (!r && e) { r = this->t17 < other.t17; e = !r && !(other.t17 < this->t17); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		if (!r && e) { r = this->t16 > other.t16; e = !r && !(other.t16 > this->t16); }  \
		if (!r && e) { r = this->t17 > other.t17; e = !r && !(other.t17 > this->t17); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); swap(this->t16, other.t16); swap(this->t17, other.t17); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)), t16(std::get<16 - 1>(t)), t17(std::get<17 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	template<> T16 &get<16 - 1>() { return this->t16; }  \
	template<> T16 const &get<16 - 1>() const { return this->t16; }  \
	template<> T17 &get<17 - 1>() { return this->t17; }  \
	template<> T17 const &get<17 - 1>() const { return this->t17; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
		swap(this->t16, get<16 - 1>(other));  \
		swap(this->t17, get<17 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_18TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15, T16, t16, T17, t17, T18, t18)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(18);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	template<> struct element_type<16 - 1> { typedef T16 type; };  \
	template<> struct element_type<17 - 1> { typedef T17 type; };  \
	template<> struct element_type<18 - 1> { typedef T18 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15; T16 t16; T17 t17; T18 t18;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15(), t16(), t17(), t18() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15, T16 const &t16, T17 const &t17, T18 const &t18)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15), t16(t16), t17(t17), t18(t18) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; r = r && this->t16 == other.t16; r = r && this->t17 == other.t17; r = r && this->t18 == other.t18; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; r = r || this->t16 != other.t16; r = r || this->t17 != other.t17; r = r || this->t18 != other.t18; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; r = r && this->t16 <= other.t16; r = r && this->t17 <= other.t17; r = r && this->t18 <= other.t18; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; r = r && this->t16 >= other.t16; r = r && this->t17 >= other.t17; r = r && this->t18 >= other.t18; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		if (!r && e) { r = this->t16 < other.t16; e = !r && !(other.t16 < this->t16); }  \
		if (!r && e) { r = this->t17 < other.t17; e = !r && !(other.t17 < this->t17); }  \
		if (!r && e) { r = this->t18 < other.t18; e = !r && !(other.t18 < this->t18); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		if (!r && e) { r = this->t16 > other.t16; e = !r && !(other.t16 > this->t16); }  \
		if (!r && e) { r = this->t17 > other.t17; e = !r && !(other.t17 > this->t17); }  \
		if (!r && e) { r = this->t18 > other.t18; e = !r && !(other.t18 > this->t18); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); swap(this->t16, other.t16); swap(this->t17, other.t17); swap(this->t18, other.t18); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)), t16(std::get<16 - 1>(t)), t17(std::get<17 - 1>(t)), t18(std::get<18 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	template<> T16 &get<16 - 1>() { return this->t16; }  \
	template<> T16 const &get<16 - 1>() const { return this->t16; }  \
	template<> T17 &get<17 - 1>() { return this->t17; }  \
	template<> T17 const &get<17 - 1>() const { return this->t17; }  \
	template<> T18 &get<18 - 1>() { return this->t18; }  \
	template<> T18 const &get<18 - 1>() const { return this->t18; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
		swap(this->t16, get<16 - 1>(other));  \
		swap(this->t17, get<17 - 1>(other));  \
		swap(this->t18, get<18 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_19TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15, T16, t16, T17, t17, T18, t18, T19, t19)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(19);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	template<> struct element_type<16 - 1> { typedef T16 type; };  \
	template<> struct element_type<17 - 1> { typedef T17 type; };  \
	template<> struct element_type<18 - 1> { typedef T18 type; };  \
	template<> struct element_type<19 - 1> { typedef T19 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15; T16 t16; T17 t17; T18 t18; T19 t19;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15(), t16(), t17(), t18(), t19() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15, T16 const &t16, T17 const &t17, T18 const &t18, T19 const &t19)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15), t16(t16), t17(t17), t18(t18), t19(t19) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; r = r && this->t16 == other.t16; r = r && this->t17 == other.t17; r = r && this->t18 == other.t18; r = r && this->t19 == other.t19; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; r = r || this->t16 != other.t16; r = r || this->t17 != other.t17; r = r || this->t18 != other.t18; r = r || this->t19 != other.t19; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; r = r && this->t16 <= other.t16; r = r && this->t17 <= other.t17; r = r && this->t18 <= other.t18; r = r && this->t19 <= other.t19; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; r = r && this->t16 >= other.t16; r = r && this->t17 >= other.t17; r = r && this->t18 >= other.t18; r = r && this->t19 >= other.t19; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		if (!r && e) { r = this->t16 < other.t16; e = !r && !(other.t16 < this->t16); }  \
		if (!r && e) { r = this->t17 < other.t17; e = !r && !(other.t17 < this->t17); }  \
		if (!r && e) { r = this->t18 < other.t18; e = !r && !(other.t18 < this->t18); }  \
		if (!r && e) { r = this->t19 < other.t19; e = !r && !(other.t19 < this->t19); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		if (!r && e) { r = this->t16 > other.t16; e = !r && !(other.t16 > this->t16); }  \
		if (!r && e) { r = this->t17 > other.t17; e = !r && !(other.t17 > this->t17); }  \
		if (!r && e) { r = this->t18 > other.t18; e = !r && !(other.t18 > this->t18); }  \
		if (!r && e) { r = this->t19 > other.t19; e = !r && !(other.t19 > this->t19); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); swap(this->t16, other.t16); swap(this->t17, other.t17); swap(this->t18, other.t18); swap(this->t19, other.t19); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)), t16(std::get<16 - 1>(t)), t17(std::get<17 - 1>(t)), t18(std::get<18 - 1>(t)), t19(std::get<19 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	template<> T16 &get<16 - 1>() { return this->t16; }  \
	template<> T16 const &get<16 - 1>() const { return this->t16; }  \
	template<> T17 &get<17 - 1>() { return this->t17; }  \
	template<> T17 const &get<17 - 1>() const { return this->t17; }  \
	template<> T18 &get<18 - 1>() { return this->t18; }  \
	template<> T18 const &get<18 - 1>() const { return this->t18; }  \
	template<> T19 &get<19 - 1>() { return this->t19; }  \
	template<> T19 const &get<19 - 1>() const { return this->t19; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
		swap(this->t16, get<16 - 1>(other));  \
		swap(this->t17, get<17 - 1>(other));  \
		swap(this->t18, get<18 - 1>(other));  \
		swap(this->t19, get<19 - 1>(other));  \
	}  \
	*/  \
}

#define DECLARE_NAMED_20TUPLE(This, T1, t1, T2, t2, T3, t3, T4, t4, T5, t5, T6, t6, T7, t7, T8, t8, T9, t9, T10, t10, T11, t11, T12, t12, T13, t13, T14, t14, T15, t15, T16, t16, T17, t17, T18, t18, T19, t19, T20, t20)  \
class This  \
{  \
public:  \
	static size_t const size = static_cast<size_t>(20);  \
	template<size_t> struct element_type;  \
	template<> struct element_type<1 - 1> { typedef T1 type; };  \
	template<> struct element_type<2 - 1> { typedef T2 type; };  \
	template<> struct element_type<3 - 1> { typedef T3 type; };  \
	template<> struct element_type<4 - 1> { typedef T4 type; };  \
	template<> struct element_type<5 - 1> { typedef T5 type; };  \
	template<> struct element_type<6 - 1> { typedef T6 type; };  \
	template<> struct element_type<7 - 1> { typedef T7 type; };  \
	template<> struct element_type<8 - 1> { typedef T8 type; };  \
	template<> struct element_type<9 - 1> { typedef T9 type; };  \
	template<> struct element_type<10 - 1> { typedef T10 type; };  \
	template<> struct element_type<11 - 1> { typedef T11 type; };  \
	template<> struct element_type<12 - 1> { typedef T12 type; };  \
	template<> struct element_type<13 - 1> { typedef T13 type; };  \
	template<> struct element_type<14 - 1> { typedef T14 type; };  \
	template<> struct element_type<15 - 1> { typedef T15 type; };  \
	template<> struct element_type<16 - 1> { typedef T16 type; };  \
	template<> struct element_type<17 - 1> { typedef T17 type; };  \
	template<> struct element_type<18 - 1> { typedef T18 type; };  \
	template<> struct element_type<19 - 1> { typedef T19 type; };  \
	template<> struct element_type<20 - 1> { typedef T20 type; };  \
	T1 t1; T2 t2; T3 t3; T4 t4; T5 t5; T6 t6; T7 t7; T8 t8; T9 t9; T10 t10; T11 t11; T12 t12; T13 t13; T14 t14; T15 t15; T16 t16; T17 t17; T18 t18; T19 t19; T20 t20;  \
	explicit This() : t1(), t2(), t3(), t4(), t5(), t6(), t7(), t8(), t9(), t10(), t11(), t12(), t13(), t14(), t15(), t16(), t17(), t18(), t19(), t20() { }  \
	explicit This(T1 const &t1, T2 const &t2, T3 const &t3, T4 const &t4, T5 const &t5, T6 const &t6, T7 const &t7, T8 const &t8, T9 const &t9, T10 const &t10, T11 const &t11, T12 const &t12, T13 const &t13, T14 const &t14, T15 const &t15, T16 const &t16, T17 const &t17, T18 const &t18, T19 const &t19, T20 const &t20)  \
		: t1(t1), t2(t2), t3(t3), t4(t4), t5(t5), t6(t6), t7(t7), t8(t8), t9(t9), t10(t10), t11(t11), t12(t12), t13(t13), t14(t14), t15(t15), t16(t16), t17(t17), t18(t18), t19(t19), t20(t20) { }  \
	bool operator==(This const &other) const { bool r = true;  r = r && this->t1 == other.t1; r = r && this->t2 == other.t2; r = r && this->t3 == other.t3; r = r && this->t4 == other.t4; r = r && this->t5 == other.t5; r = r && this->t6 == other.t6; r = r && this->t7 == other.t7; r = r && this->t8 == other.t8; r = r && this->t9 == other.t9; r = r && this->t10 == other.t10; r = r && this->t11 == other.t11; r = r && this->t12 == other.t12; r = r && this->t13 == other.t13; r = r && this->t14 == other.t14; r = r && this->t15 == other.t15; r = r && this->t16 == other.t16; r = r && this->t17 == other.t17; r = r && this->t18 == other.t18; r = r && this->t19 == other.t19; r = r && this->t20 == other.t20; return r; }  \
	bool operator!=(This const &other) const { bool r = false; r = r || this->t1 != other.t1; r = r || this->t2 != other.t2; r = r || this->t3 != other.t3; r = r || this->t4 != other.t4; r = r || this->t5 != other.t5; r = r || this->t6 != other.t6; r = r || this->t7 != other.t7; r = r || this->t8 != other.t8; r = r || this->t9 != other.t9; r = r || this->t10 != other.t10; r = r || this->t11 != other.t11; r = r || this->t12 != other.t12; r = r || this->t13 != other.t13; r = r || this->t14 != other.t14; r = r || this->t15 != other.t15; r = r || this->t16 != other.t16; r = r || this->t17 != other.t17; r = r || this->t18 != other.t18; r = r || this->t19 != other.t19; r = r || this->t20 != other.t20; return r; }  \
	bool operator<=(This const &other) const { bool r = true;  r = r && this->t1 <= other.t1; r = r && this->t2 <= other.t2; r = r && this->t3 <= other.t3; r = r && this->t4 <= other.t4; r = r && this->t5 <= other.t5; r = r && this->t6 <= other.t6; r = r && this->t7 <= other.t7; r = r && this->t8 <= other.t8; r = r && this->t9 <= other.t9; r = r && this->t10 <= other.t10; r = r && this->t11 <= other.t11; r = r && this->t12 <= other.t12; r = r && this->t13 <= other.t13; r = r && this->t14 <= other.t14; r = r && this->t15 <= other.t15; r = r && this->t16 <= other.t16; r = r && this->t17 <= other.t17; r = r && this->t18 <= other.t18; r = r && this->t19 <= other.t19; r = r && this->t20 <= other.t20; return r; }  \
	bool operator>=(This const &other) const { bool r = true;  r = r && this->t1 >= other.t1; r = r && this->t2 >= other.t2; r = r && this->t3 >= other.t3; r = r && this->t4 >= other.t4; r = r && this->t5 >= other.t5; r = r && this->t6 >= other.t6; r = r && this->t7 >= other.t7; r = r && this->t8 >= other.t8; r = r && this->t9 >= other.t9; r = r && this->t10 >= other.t10; r = r && this->t11 >= other.t11; r = r && this->t12 >= other.t12; r = r && this->t13 >= other.t13; r = r && this->t14 >= other.t14; r = r && this->t15 >= other.t15; r = r && this->t16 >= other.t16; r = r && this->t17 >= other.t17; r = r && this->t18 >= other.t18; r = r && this->t19 >= other.t19; r = r && this->t20 >= other.t20; return r; }  \
	bool operator< (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 < other.t1; e = !r && !(other.t1 < this->t1); }  \
		if (!r && e) { r = this->t2 < other.t2; e = !r && !(other.t2 < this->t2); }  \
		if (!r && e) { r = this->t3 < other.t3; e = !r && !(other.t3 < this->t3); }  \
		if (!r && e) { r = this->t4 < other.t4; e = !r && !(other.t4 < this->t4); }  \
		if (!r && e) { r = this->t5 < other.t5; e = !r && !(other.t5 < this->t5); }  \
		if (!r && e) { r = this->t6 < other.t6; e = !r && !(other.t6 < this->t6); }  \
		if (!r && e) { r = this->t7 < other.t7; e = !r && !(other.t7 < this->t7); }  \
		if (!r && e) { r = this->t8 < other.t8; e = !r && !(other.t8 < this->t8); }  \
		if (!r && e) { r = this->t9 < other.t9; e = !r && !(other.t9 < this->t9); }  \
		if (!r && e) { r = this->t10 < other.t10; e = !r && !(other.t10 < this->t10); }  \
		if (!r && e) { r = this->t11 < other.t11; e = !r && !(other.t11 < this->t11); }  \
		if (!r && e) { r = this->t12 < other.t12; e = !r && !(other.t12 < this->t12); }  \
		if (!r && e) { r = this->t13 < other.t13; e = !r && !(other.t13 < this->t13); }  \
		if (!r && e) { r = this->t14 < other.t14; e = !r && !(other.t14 < this->t14); }  \
		if (!r && e) { r = this->t15 < other.t15; e = !r && !(other.t15 < this->t15); }  \
		if (!r && e) { r = this->t16 < other.t16; e = !r && !(other.t16 < this->t16); }  \
		if (!r && e) { r = this->t17 < other.t17; e = !r && !(other.t17 < this->t17); }  \
		if (!r && e) { r = this->t18 < other.t18; e = !r && !(other.t18 < this->t18); }  \
		if (!r && e) { r = this->t19 < other.t19; e = !r && !(other.t19 < this->t19); }  \
		if (!r && e) { r = this->t20 < other.t20; e = !r && !(other.t20 < this->t20); }  \
		return r;  \
	}  \
	bool operator> (This const &other) const  \
	{  \
		bool r = false;  \
		bool e = true;  \
		(void)e;  \
		if (!r && e) { r = this->t1 > other.t1; e = !r && !(other.t1 > this->t1); }  \
		if (!r && e) { r = this->t2 > other.t2; e = !r && !(other.t2 > this->t2); }  \
		if (!r && e) { r = this->t3 > other.t3; e = !r && !(other.t3 > this->t3); }  \
		if (!r && e) { r = this->t4 > other.t4; e = !r && !(other.t4 > this->t4); }  \
		if (!r && e) { r = this->t5 > other.t5; e = !r && !(other.t5 > this->t5); }  \
		if (!r && e) { r = this->t6 > other.t6; e = !r && !(other.t6 > this->t6); }  \
		if (!r && e) { r = this->t7 > other.t7; e = !r && !(other.t7 > this->t7); }  \
		if (!r && e) { r = this->t8 > other.t8; e = !r && !(other.t8 > this->t8); }  \
		if (!r && e) { r = this->t9 > other.t9; e = !r && !(other.t9 > this->t9); }  \
		if (!r && e) { r = this->t10 > other.t10; e = !r && !(other.t10 > this->t10); }  \
		if (!r && e) { r = this->t11 > other.t11; e = !r && !(other.t11 > this->t11); }  \
		if (!r && e) { r = this->t12 > other.t12; e = !r && !(other.t12 > this->t12); }  \
		if (!r && e) { r = this->t13 > other.t13; e = !r && !(other.t13 > this->t13); }  \
		if (!r && e) { r = this->t14 > other.t14; e = !r && !(other.t14 > this->t14); }  \
		if (!r && e) { r = this->t15 > other.t15; e = !r && !(other.t15 > this->t15); }  \
		if (!r && e) { r = this->t16 > other.t16; e = !r && !(other.t16 > this->t16); }  \
		if (!r && e) { r = this->t17 > other.t17; e = !r && !(other.t17 > this->t17); }  \
		if (!r && e) { r = this->t18 > other.t18; e = !r && !(other.t18 > this->t18); }  \
		if (!r && e) { r = this->t19 > other.t19; e = !r && !(other.t19 > this->t19); }  \
		if (!r && e) { r = this->t20 > other.t20; e = !r && !(other.t20 > this->t20); }  \
		return r;  \
	}  \
	void swap(This &other) { using std::swap; swap(this->t1, other.t1); swap(this->t2, other.t2); swap(this->t3, other.t3); swap(this->t4, other.t4); swap(this->t5, other.t5); swap(this->t6, other.t6); swap(this->t7, other.t7); swap(this->t8, other.t8); swap(this->t9, other.t9); swap(this->t10, other.t10); swap(this->t11, other.t11); swap(this->t12, other.t12); swap(this->t13, other.t13); swap(this->t14, other.t14); swap(this->t15, other.t15); swap(this->t16, other.t16); swap(this->t17, other.t17); swap(this->t18, other.t18); swap(this->t19, other.t19); swap(this->t20, other.t20); }  \
	friend void swap(This &a, This &b) { return a.swap(b); }  \
	\
	/*  \
	explicit This(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> const &t)  \
		: t1(std::get<1 - 1>(t)), t2(std::get<2 - 1>(t)), t3(std::get<3 - 1>(t)), t4(std::get<4 - 1>(t)), t5(std::get<5 - 1>(t)), t6(std::get<6 - 1>(t)), t7(std::get<7 - 1>(t)), t8(std::get<8 - 1>(t)), t9(std::get<9 - 1>(t)), t10(std::get<10 - 1>(t)), t11(std::get<11 - 1>(t)), t12(std::get<12 - 1>(t)), t13(std::get<13 - 1>(t)), t14(std::get<14 - 1>(t)), t15(std::get<15 - 1>(t)), t16(std::get<16 - 1>(t)), t17(std::get<17 - 1>(t)), t18(std::get<18 - 1>(t)), t19(std::get<19 - 1>(t)), t20(std::get<20 - 1>(t)) { }  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> >::type       &get()      ;  \
	template<size_t I> typename std::tuple_element<I, std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> >::type const &get() const;  \
	operator std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20>() const { return std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20>(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20); }  \
	template<> T1 &get<1 - 1>() { return this->t1; }  \
	template<> T1 const &get<1 - 1>() const { return this->t1; }  \
	template<> T2 &get<2 - 1>() { return this->t2; }  \
	template<> T2 const &get<2 - 1>() const { return this->t2; }  \
	template<> T3 &get<3 - 1>() { return this->t3; }  \
	template<> T3 const &get<3 - 1>() const { return this->t3; }  \
	template<> T4 &get<4 - 1>() { return this->t4; }  \
	template<> T4 const &get<4 - 1>() const { return this->t4; }  \
	template<> T5 &get<5 - 1>() { return this->t5; }  \
	template<> T5 const &get<5 - 1>() const { return this->t5; }  \
	template<> T6 &get<6 - 1>() { return this->t6; }  \
	template<> T6 const &get<6 - 1>() const { return this->t6; }  \
	template<> T7 &get<7 - 1>() { return this->t7; }  \
	template<> T7 const &get<7 - 1>() const { return this->t7; }  \
	template<> T8 &get<8 - 1>() { return this->t8; }  \
	template<> T8 const &get<8 - 1>() const { return this->t8; }  \
	template<> T9 &get<9 - 1>() { return this->t9; }  \
	template<> T9 const &get<9 - 1>() const { return this->t9; }  \
	template<> T10 &get<10 - 1>() { return this->t10; }  \
	template<> T10 const &get<10 - 1>() const { return this->t10; }  \
	template<> T11 &get<11 - 1>() { return this->t11; }  \
	template<> T11 const &get<11 - 1>() const { return this->t11; }  \
	template<> T12 &get<12 - 1>() { return this->t12; }  \
	template<> T12 const &get<12 - 1>() const { return this->t12; }  \
	template<> T13 &get<13 - 1>() { return this->t13; }  \
	template<> T13 const &get<13 - 1>() const { return this->t13; }  \
	template<> T14 &get<14 - 1>() { return this->t14; }  \
	template<> T14 const &get<14 - 1>() const { return this->t14; }  \
	template<> T15 &get<15 - 1>() { return this->t15; }  \
	template<> T15 const &get<15 - 1>() const { return this->t15; }  \
	template<> T16 &get<16 - 1>() { return this->t16; }  \
	template<> T16 const &get<16 - 1>() const { return this->t16; }  \
	template<> T17 &get<17 - 1>() { return this->t17; }  \
	template<> T17 const &get<17 - 1>() const { return this->t17; }  \
	template<> T18 &get<18 - 1>() { return this->t18; }  \
	template<> T18 const &get<18 - 1>() const { return this->t18; }  \
	template<> T19 &get<19 - 1>() { return this->t19; }  \
	template<> T19 const &get<19 - 1>() const { return this->t19; }  \
	template<> T20 &get<20 - 1>() { return this->t20; }  \
	template<> T20 const &get<20 - 1>() const { return this->t20; }  \
	void swap(std::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> &other)  \
	{  \
		using std::get;  \
		using std::swap;  \
		swap(this->t1, get<1 - 1>(other));  \
		swap(this->t2, get<2 - 1>(other));  \
		swap(this->t3, get<3 - 1>(other));  \
		swap(this->t4, get<4 - 1>(other));  \
		swap(this->t5, get<5 - 1>(other));  \
		swap(this->t6, get<6 - 1>(other));  \
		swap(this->t7, get<7 - 1>(other));  \
		swap(this->t8, get<8 - 1>(other));  \
		swap(this->t9, get<9 - 1>(other));  \
		swap(this->t10, get<10 - 1>(other));  \
		swap(this->t11, get<11 - 1>(other));  \
		swap(this->t12, get<12 - 1>(other));  \
		swap(this->t13, get<13 - 1>(other));  \
		swap(this->t14, get<14 - 1>(other));  \
		swap(this->t15, get<15 - 1>(other));  \
		swap(this->t16, get<16 - 1>(other));  \
		swap(this->t17, get<17 - 1>(other));  \
		swap(this->t18, get<18 - 1>(other));  \
		swap(this->t19, get<19 - 1>(other));  \
		swap(this->t20, get<20 - 1>(other));  \
	}  \
	*/  \
}

