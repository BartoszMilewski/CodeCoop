#if !defined (BOUNDEDQUEUE_H)
#define BOUNDEDQUEUE_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "Synchro.h"
#include <deque>

// Producer/Consumer queue

template<class T>
class BoundedQueue
{
public:
	BoundedQueue (int bound);
	~BoundedQueue ();
	void Put (T const & data);
	T Get ();
private:
	Win::CritSection	_dataSection;
	Win::Semaphore		_spaceAvailable;
	Win::Semaphore		_dataAvailable;

	std::deque<T>		_data;
	int					_bound;
};

template<class T>
BoundedQueue<T>::BoundedQueue (int bound)
	: _spaceAvailable (bound, bound),
	  _dataAvailable (bound),
	  _bound (bound)
{}

template<class T>
BoundedQueue<T>::~BoundedQueue ()
{
}

template<class T>
void BoundedQueue<T>::Put (T const & data)
{
	_spaceAvailable.Wait ();

	{
		Win::Lock lock (_dataSection);
		assert (_data.size () < _bound);
		_data.push_back (data);
	}
	_dataAvailable.Increment ();
}

template<class T>
T BoundedQueue<T>::Get ()
{
	T data;
	_dataAvailable.Wait ();

	{
		Win::Lock lock (_dataSection);
		assert (_data.size () > 0);
		data = _data.front ();
		_data.pop_front ();
	}
	_spaceAvailable.Increment ();
	return data;
}

#endif
