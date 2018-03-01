#if !defined (ARRAY_VECTOR_H)
#define ARRAY_VECTOR_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2006
//------------------------------------

#include <auto_array.h>
#include <iterator>
#include <cassert>

template <class T> 
class ref_vector
{
public:

    explicit ref_vector (size_t capacity = 0);
    ~ref_vector () { delete []_arr; }

    size_t  size () const { return _end; }
	void assign (size_t i, T & val);

	// stack access
	void push_back (T & val);
    T pop_back ();

	// memory management
	size_t  capacity () const { return _capacity; }
    void reserve (size_t count);
    void reserve_more (size_t count);
	void resize (unsigned int newSize);


	// list access
	void insert (size_t i, T & p);

protected:
	void init (size_t size, T * arr);
    void grow (size_t reqCapacity);
	// indexed access
    T const & ref_at (size_t i) const;
    T & ref_at (size_t i);
    T const & ref_back () const;
    T & ref_back ();
	// iterators
	typedef T * iterator;
	typedef T const * const_iterator;
	typedef T * reverse_iterator;
	typedef T const * const_reverse_iterator;
	iterator ref_begin() { return &_arr [0]; }
	iterator ref_end()   { return &_arr [_end]; }
	const_iterator ref_begin() const  { return &_arr [0]; }
	const_iterator ref_end() const    { return &_arr [_end]; }
	reverse_iterator ref_rbegin() { return &_arr [_end]; }
	reverse_iterator ref_rend()   { return &_arr [0] ; }
	const_reverse_iterator ref_rbegin() const  { return &_arr [_end]; }
	const_reverse_iterator ref_rend() const    { return &_arr [0]; }

protected:
    T	  * _arr;
    size_t	_capacity;
    size_t	_end;
};

template <class T>
ref_vector<T>::ref_vector (size_t capacity)
    : _capacity (capacity), _end (0), _arr (0)
{
    if (_capacity > 0)
        _arr = new T [_capacity];
}

template <class T>
void ref_vector<T>::init (size_t count, T * arr)
{
    delete [] _arr;
    _capacity = count;
    _end = count;
    _arr = arr;
}

template <class T>
inline T const & ref_vector<T>::ref_at (size_t i) const
{
    assert (i < _end);
    return _arr [i];
}

template <class T>
inline T & ref_vector<T>::ref_at (size_t i)
{
    assert (i < _end);
    return _arr [i];
}

template <class T>
inline void ref_vector<T>::assign (size_t i, T & val)
{
	assert (i < _end);
	_arr [i] = val;
}

template <class T>
void ref_vector<T>::push_back (T & val)
{
    reserve_more (1);
    _arr [_end++] = val;
}

template <class T>
inline T ref_vector <T>::pop_back () 
{
	assert (_end != 0);
	return _arr [--_end]; 
}

template <class T>
inline T const & ref_vector<T>::ref_back () const
{
	assert (_end != 0);
    return _arr [_end - 1];
}

template <class T>
inline T & ref_vector<T>::ref_back ()
{
	assert (_end != 0);
    return _arr [_end - 1];
}

template <class T>
inline void ref_vector <T>::reserve (size_t count)
{
	if (count > _capacity)
	    grow (count);
}

template <class T>
inline void ref_vector <T>::reserve_more (size_t count)
{
	if (_end + count > _capacity)
	    grow (_end + count);
}

template <class T>
inline void ref_vector<T>::resize (unsigned int newSize)
{
	if ( newSize > _capacity)
		grow (newSize);
	if ( _end > newSize)
	{
        // allocate New array
        T * arrNew;
		if (newSize > 0)
			arrNew = new T [newSize];
		else
			arrNew = 0;
        // transfer all entries
        for (size_t i = 0; i < newSize; i++)
			arrNew [i] = _arr [i];
        _capacity = newSize;
        // free old memory
        delete []_arr;
        // substitute New array for old array
       _arr = arrNew;
	}		
    _end = newSize;
}

template <class T>
void ref_vector<T>::insert (size_t idx, T & p)
{
	assert (idx <= _end);
	reserve_more (1);
	// Ripple copy
	for (size_t i = _end; i >= idx + 1; i--)
	{
		_arr [i] = _arr [i - 1];
	}
    _arr [idx] = p;
    _end++;
}


// protected method
template <class T>
void ref_vector<T>::grow (size_t reqCapacity)
{
    size_t newCapacity = 2 * _capacity;
    if (reqCapacity > newCapacity)
        newCapacity = reqCapacity;
    // allocate New array
    T * arrNew = new T [newCapacity];
    // transfer all entries
    for (size_t i = 0; i < _capacity; i++)
        arrNew [i] = _arr [i];
    _capacity = newCapacity;
    // free old memory
    delete []_arr;
    // substitute New array for old array
    _arr = arrNew;
}


//
// Dynamic array of smart arrays
// Ownership transfer semantics
//

template<class T> class const_array_iterator;

template<class T>
class array_iterator: public std::iterator<std::random_access_iterator_tag, T * >
{
	friend const_array_iterator<T>;
public:
	array_iterator () : _pp (0) {}
	array_iterator (auto_array<T> * pp) : _pp (pp) {}
	bool operator != (auto_array<T> const * p) const 
		{ return p != _pp; }
	bool operator != (array_iterator<T> const & it) const 
		{ return it._pp != _pp; }
	bool operator == (array_iterator<T> const & it) const 
		{ return it._pp == _pp; }
	array_iterator const & operator++ (int) {	return _pp++; }
	array_iterator operator++ () { return ++_pp; }
	T * operator * () { return _pp->get (); }
private:
	auto_array<T> * _pp;
};

template<class T>
class const_array_iterator: public std::iterator<std::random_access_iterator_tag, T const * >
{
public:
	const_array_iterator () : _pp (0) {}
	const_array_iterator (auto_array<T> const * pp) : _pp (pp) {}
	const_array_iterator (array_iterator<T> & it) : _pp (it._pp) {}
	bool operator != (auto_array<T> const * p) const 
		{ return p != _pp; }
	bool operator != (const_array_iterator<T> const & it) const 
		{ return it._pp != _pp; }
	bool operator == (const_array_iterator<T> const & it) const 
		{ return it._pp == _pp; }
	const_array_iterator const & operator++ (int) {	return _pp++; }
	const_array_iterator const operator++ () { return ++_pp; }
	T const * operator * () { return _pp->get (); }
private:
	auto_array<T> const * _pp;
};

template <class T> 
class array_vector: public ref_vector< auto_array<T> >
{
	typedef ref_vector< auto_array<T> > Parent;
public:
    explicit array_vector (size_t capacity = 0): Parent (capacity) {}
    void clear ();
    T const * operator [] (size_t i) const { return ref_at (i).get (); }
    T * operator [] (size_t i) { return ref_at (i).get (); }
	void erase (size_t idx);
	// iterators
	typedef array_iterator<T> iterator;
	typedef const_array_iterator<T> const_iterator;
	iterator begin () { return Parent::ref_begin (); }
	iterator end () { return Parent::ref_end (); }
	const_iterator begin () const { return Parent::ref_begin (); }
	const_iterator end () const { return Parent::ref_end (); }
};

template <class T>
void array_vector <T>::clear ()
{
    for (size_t i = 0; i < _end; i++)
    {
        _arr [i].reset ();
    }
    _end = 0;
}

template <class T>
void array_vector<T>::erase (size_t idx)
{
	assert (idx < _end);
	// Delete item
	delete _arr [idx].release ();
	// Compact array
	for (size_t i = idx; i < _end - 1; i++)
	{
		// strong assignment
		_arr [i] = _arr [i + 1];
	}
	_end--;
}

class StringArray: public array_vector<char>
{
public:
	typedef array_vector<char> Parent;
	typedef array_vector<char>::iterator iterator;
	typedef array_vector<char>::const_iterator const_iterator;

public:
	void push_back (std::string & str)
	{
		auto_array<char> s (new char [str.size () + 1]);
		memcpy (s.get (), str.c_str (), str.size () + 1);
		Parent::push_back (s);
	}
	void push_back (char const * str)
	{
		size_t len = strlen (str);
		auto_array<char> s (new char [len + 1]);
		memcpy (s.get (), str, len + 1);
		Parent::push_back (s);
	}
	void push_back (char const * str, int len)
	{
		auto_array<char> s (new char [len + 1]);
		memcpy (s.get (), str, len + 1);
		Parent::push_back (s);
	}
	char const ** get () const { return reinterpret_cast<char const **>(_arr); }
};

#endif
