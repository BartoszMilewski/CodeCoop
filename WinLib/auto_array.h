#if !defined AUTO_ARRAY_H
#define AUTO_ARRAY_H
//------------------------------------
//  (c) Reliable Software, 1996-2002
//------------------------------------

template<class T>
class auto_array
{
	template<class U> struct auto_array_ref
	{
		auto_array<U> & _aa;
		auto_array_ref (auto_array<U> & aa) : _aa (aa) {}
	};
public:
	typedef T element_type;

	explicit auto_array (size_t size)
		: _a (size != 0? new T [size]: 0) 
	{}

	explicit auto_array (T * p = 0) : _a (p) {}

	// "transfer" constructor
	auto_array (auto_array & aSrc)
	{
		_a = aSrc._a;
		aSrc._a = 0;
	}

	auto_array & operator= (auto_array & aSrc)
	{
		if (this != &aSrc)
		{
			_a = aSrc._a;
			aSrc._a = 0;
		}
		return *this;
	}

	template<class U> 
	auto_array (auto_array<U> & aSrc)
	{
		_a = aSrc._a; // implicit cast
		aSrc._a = 0;
	}

	// assignment of derived class
	template<class U> 
	auto_array & operator= (auto_array<U> & aSrc)
	{
		if (this != &aSrc)
		{
			_a = aSrc._a;
			aSrc._a = 0;
		}
		return *this;
	}

	~auto_array () { delete []_a; }

	T const & operator [] (int i) const { return _a [i]; }

	T & operator [] (int i) { return _a [i]; }

	T * get () const { return _a; }

	T * release ()
	{
		T * tmp = _a;
		_a = 0;
		return tmp;
	}

	void reset (T * p = 0) throw ()
	{
		if (p != _a)
		{
			delete []_a;
			_a = p;
		}
	}

	auto_array (auto_array_ref<T> apr)
		: _a (apr._aa.release ())
	{}

	template<class U>
	operator auto_array_ref<U> ()
	{
		return auto_array_ref<U> (*this);
	}

	template<class U>
	operator auto_array<U> ()
	{
		return auto_array<U> (release ());
	}
private:
	T * _a;
};

#endif
