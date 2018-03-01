#if !defined (OWNERVECTOR_H)
#define OWNERVECTOR_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "OutputSink.h" // for debugging only
#include <auto_vector.h>
#include <Dbg/Assert.h>

template<class T>
class OwnerVector
{
public:
	OwnerVector () : _count (0) {}
	unsigned Count () const { return _count; }
	bool AddNoDup (std::unique_ptr<T> element);
	void Remove (T * element);
	std::unique_ptr<T> Extract(T * element);
	void clear ()
	{
		_elements.resize (0);
		_count = 0;
	}
	std::unique_ptr<T> PopBack ();
	void Compact ();

	typedef typename auto_vector<T>::iterator iterator;
	typedef typename auto_vector<T>::const_iterator const_iterator;
	iterator begin () { return _elements.begin (); }
	iterator end () { return _elements.end (); }
	const_iterator begin () const { return _elements.begin (); }
	const_iterator end () const { return _elements.end (); }
private:
	int Find (T * element);
private:
	auto_vector<T>	_elements;
	unsigned		_count;
};

template<class T>
void OwnerVector<T>::Compact ()
{
	// shrink when count goes to zero
	if (_count == 0)
		while (_elements.size () != 0)
			_elements.pop_back ();
}

// returns false if duplicate
template<class T>
bool OwnerVector<T>::AddNoDup (std::unique_ptr<T> element)
{
	unsigned emptySlot = _elements.size ();
	for (unsigned i = 0; i < _elements.size (); ++i)
	{
		if (_elements [i] == 0)
			emptySlot = i;
		else if (_elements [i] == element.get () || *_elements [i] == *element.get ())
		{
			return false;	// no duplicates
		}
	}
	if (emptySlot < _elements.size ())
		_elements.assign (emptySlot, element);
	else
		_elements.push_back (element);
	++_count;
	return true;
}

// doesn't invalidate iterator
template<class T>
void OwnerVector<T>::Remove (T * element)
{
	Assert (element != 0);
	unsigned i = Find(element);
	Assert (i != -1);
	_elements.assign_direct (i, 0);
	--_count;
}

template<class T>
std::unique_ptr<T> OwnerVector<T>::Extract(T * element)
{
	Assert (element != 0);
	unsigned i = Find(element);
	Assert (i != -1);
	--_count;
	return _elements.extract(i);
}

template<class T>
int OwnerVector<T>::Find (T * element)
{
	Assert (element != 0);
	unsigned i = 0;
	while (i < _elements.size ())
	{
		T * curElement = _elements[i];
		if (curElement != 0 && curElement == element 
			|| (curElement != 0 && *curElement == *element))
		{
			return i;
		}
		++i;
	}
	return -1;
}

template<class T>
std::unique_ptr<T> OwnerVector<T>::PopBack ()
{
	if (_elements.back () != 0)
		--_count;
	return _elements.pop_back ();
}

#endif
