#if !defined (SYNCHRO_H)
#define SYNCHRO_H
//----------------------------------
// (c) Reliable Software 2000 - 2007
//----------------------------------
#include <Win/SysHandle.h>

namespace Win
{
	class CritSection
	{
	public:
		CritSection (bool delayInit)
			: _count (0xffffffff)
		{}
        CritSection();
        ~CritSection();
		// Only call when delayInit constructor was used!
        void Init();
		unsigned Acquire () 
		{
			::EnterCriticalSection (&_critSection);
			++_count;
			Assert (_count != 0);
			return _count;
		}
		void Release () 
		{ 
			Assert (_count != 0);
			--_count;
			::LeaveCriticalSection (&_critSection);
		}
	private:
		CritSection (CritSection const &);
		CritSection & operator= (CritSection const &);
	private:
		CRITICAL_SECTION	_critSection;
		unsigned			_count;
	};

	class Lock 
	{
	public:
		Lock (CritSection & critSect) 
			: _critSect (critSect) 
		{
			_critSect.Acquire ();
		}
		~Lock ()
		{
			_critSect.Release ();
		}
	private:
		CritSection & _critSect;
	};
	
	class Unlock
	{
	public:
		// release section only if it's been already entered
		Unlock (CritSection & critSect)
			: _critSect (critSect)
		{
			_count = _critSect.Acquire ();
			if (_count > 1)
				_critSect.Release ();
			_critSect.Release ();
		}
		~Unlock ()
		{
			if (_count > 1)
				_critSect.Acquire ();
		}
	private:
		CritSection & _critSect;
		unsigned _count;
	};

	class LockPtr
	{
	public:
		LockPtr (CritSection * critSect) 
			: _critSect (critSect) 
		{
			if (_critSect)
				_critSect->Acquire ();
		}
		~LockPtr ()
		{
			if (_critSect)
				_critSect->Release ();
		}
	private:
		CritSection * _critSect;
	};
	
	class UnlockPtr
	{
	public:
		// release section only if it's been already entered
		UnlockPtr (CritSection * critSect)
			: _critSect (critSect)
		{
			if (_critSect)
			{
				_count = _critSect->Acquire ();
				if (_count > 1)
					_critSect->Release ();
				_critSect->Release ();
			}
		}
		~UnlockPtr ()
		{
			if (_critSect != 0 && _count > 1)
				_critSect->Acquire ();
		}
	private:
		CritSection	*	_critSect;
		unsigned		_count;
	};

	class Mutex: public Sys::AutoHandle
	{
	public:
		Mutex (char const * name = 0, bool ownInitially = false)
			: Sys::AutoHandle (::CreateMutex (0, ownInitially? TRUE: FALSE, name))
		{
			Win::ClearError ();
		}
		void Release ()
		{
			ReleaseMutex (H ());
		}
	};

	class MutexLock
	{
	public:
		MutexLock (Mutex & mutex, unsigned timeout = INFINITE)
			: _mutex (mutex), _status(0)
		{
			_status = ::WaitForSingleObject (_mutex.ToNative (), timeout);
		}
		~MutexLock ()
		{
			_mutex.Release ();
		}
		bool WasAbandoned () const { return _status == WAIT_ABANDONED; }
		bool WasTimeout () const { return _status == WAIT_TIMEOUT; }
	private:
		Mutex & _mutex;
		DWORD	_status;
	};

	class Event: public Sys::AutoHandle
	{
	public:
		static const int NoTimeout = INFINITE;
		// start in non-signaled state (red light)
		// auto reset after every Wait
		Event ()
			: Sys::AutoHandle (::CreateEvent (0, false, false, 0))
		{
			if (IsNull ())
				throw Win::Exception ("Cannot create semaphore");
		}
		// put into signaled state: release waiting thread
		void Release ()
		{
			::SetEvent (H ());
		}
		// return true if signaled, false if timeout
		bool Wait (int timeout = NoTimeout)
		{
			// Wait until event is in signaled (green) state
			return ::WaitForSingleObject(H (), timeout) == WAIT_OBJECT_0;
		}

	protected:
		Event (HANDLE h)
			: Sys::AutoHandle (h) 
		{}
	};

	// These can be shared between processes
	class NewEvent: public Event
	{
	public:
		NewEvent (std::string const & name)
			: Event (::CreateEvent (0, false, false, name.c_str ())),
			  _name (name)
		{
			if (IsNull ())
				throw Win::Exception ("Cannot create event", name.c_str ());
		}
		// Generate unique name
		NewEvent ();
		std::string const & GetName () const { return _name; }
	private:
		std::string _name;
	};

	class ExistingEvent: public Event
	{
	public:
		ExistingEvent (std::string const & name, bool quiet = false)
			: Event (::OpenEvent (EVENT_ALL_ACCESS | EVENT_MODIFY_STATE, 
								  FALSE, 
								  name.c_str ()))
		{
			if (IsNull () && !quiet)
				throw Win::Exception ("Cannot open event", name.c_str ());
		}
	};

	class Semaphore: public Sys::AutoHandle
	{
	public:
		Semaphore (long maxCount, long initCount = 0)
			: Sys::AutoHandle (::CreateSemaphore (0, initCount, maxCount, 0))
		{
			if (IsNull ())
				throw Win::Exception ("Cannot create semaphore");
		}
		void Increment (int count = 1)
		{
			// increment semaphore count (and release waiting thread)
			if (::ReleaseSemaphore (H (), count, 0) == FALSE)
				throw Win::Exception ("Releasing semaphore failed");
		}
		void Wait ()
		{
			// Wait until count greated than zero and decrement count
			::WaitForSingleObject(H (), INFINITE);
		}
	};

	class TrafficLight : public Sys::AutoHandle
	{
	public:
		// Start in non-signaled state (red light)
		// Manual reset
		TrafficLight (char const *name = 0, bool existingLight = false)
			: Sys::AutoHandle(existingLight ? ::OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, false, name) :
										  ::CreateEvent (0, true, false, name))
		{
			if (IsNull ())
				throw Win::Exception ("Cannot create event");
		}

		// put into signaled state
		void GreenLight () 
		{ 
			::SetEvent (H ()); 
		}

		// put into non-signaled state
		void RedLight () 
		{ 
			::ResetEvent (H ()); 
		}

		void Wait (int timeout = INFINITE)
		{
			// Wait until event is in signaled (green) state
			::WaitForSingleObject(H (), timeout);
		}

	};

#if 0	//	CreateWaitableTimer not available on Win95
	class WaitableTimer : public Sys::AutoHandle
	{
	public:
		WaitableTimer ();
		WaitableTimer (std::string const & name);

		void Set (unsigned int timeout);	// Timeout in miliseconds
		bool Wait ();	// Returns true if timer signaled otherwise wait failed

		std::string const & GetName () const { return _name; }

	private:
		std::string	_name;
	};
#endif

	class AtomicCounterPtr
	{
	public:
		AtomicCounterPtr (volatile long * pCounter) 
			: _pCounter(pCounter) 
		{}

		long Inc ()
		{
			// Return the sign (or zero) of the New value
			return ::InterlockedIncrement (_pCounter);
		}

		long Dec ()
		{
			// Return the sign (or zero) of the New value
			return ::InterlockedDecrement (_pCounter);
		}

		bool IsNonZero () const
		{
			return *_pCounter != 0;
		}

		bool IsNonZeroReset ()
		{
			return ::InterlockedExchange (_pCounter, 0) != 0;
		}

		void Reset()
		{
			::InterlockedExchange (_pCounter, 0);
		}

		bool IsNonZeroSet (long newVal)
		{
			return ::InterlockedExchange (_pCounter, newVal) != 0;
		}
  
		void Set (long newVal)
		{
			::InterlockedExchange (_pCounter, newVal);
		}
	private:
		volatile long * const _pCounter;
	};

	class AtomicCounter : public AtomicCounterPtr
	{
	public:
		AtomicCounter ()
			: AtomicCounterPtr (&_counter), _counter (0)
		{}
		bool IsNonZero () const
		{
			return _counter != 0;
		}

	private:
		volatile long _counter;
	};

	// An object that can be shared only by two threads
	// Uses lock-free algorithm
	class SharedByTwo
	{
	public:
		SharedByTwo () : _isShared (0) {}
		void Acquire ()
		{
			Assume (_isShared == 0, "Win::SharedByTwo::Acquire");
			::InterlockedIncrement (&_isShared);
		}
		// returns true if needs deletion
		bool Release ()
		{
			// Atomically do the following:
			// val = _isShared;
			// if (_isShared == 1)
			//     _isShared = 0;
			// return val == 0;
			return ::InterlockedCompareExchange (&_isShared, 0, 1) == 0;
		}
	private:
		mutable volatile long _isShared;
	};

	template<typename T>
	class SharedByTwoOwner
	{
	public:
		SharedByTwoOwner (T * shared = 0): _shared (shared) 
		{}
		SharedByTwoOwner (SharedByTwoOwner<T> const & p)
			: _shared (p._shared)
		{
			if (_shared != 0)
				_shared->Acquire ();
		}
		~SharedByTwoOwner ()
		{
			Release ();
		}
		SharedByTwoOwner const & operator= (SharedByTwoOwner<T> const & p)
		{
			if (this != &p)
			{
				Release ();
				_shared = p._shared;
				if (_shared != 0)
					_shared->Acquire ();
			}
			return *this;
		}
		bool empty () const { return _shared == 0;	}
		T * operator-> () { return _shared; }

		template<typename U>
			U * GetPtr ()
		{
			return static_cast<U *> (_shared);
		}
		template<typename U>
			U const * GetPtr () const
		{
			return static_cast<U *> (_shared);
		}
	protected:
		void Release ()
		{
			if (_shared && _shared->Release ())
			{
				delete _shared;
				_shared = 0;
			}
		}
	private:
		T * _shared;
	};
}

#endif
