#if !defined ACTIVE_H
#define ACTIVE_H
//------------------------------------
//  (c) Reliable Software, 1996-2003
//------------------------------------

//------------------------------------------------------
// Inside the Run loop you must keep checking _isDying
//    if (IsDying ())
//         return;
// If you have any Waits for Events,
// implement FlushThread to Release the thread
//
// DO NOT EMBED OR CREATE ActiveObject ON THE STACK!
// Use auto_active to manage its release
//-------------------------------------------------------

#include <Win/Thread.h>
#include <Win/Synchro.h>

class ActiveObject
{
	// As soon as the compiler allows, make destructor protected
	template<class T>
	friend class auto_active;
public:
    ActiveObject ();
	// Kill will stop the thread and cause the eventual
	// (asynchronous) deletion of the ActiveObject
    void Kill (unsigned timeoutMs = 0) throw ();
	void KillSynchronous () throw () { Kill (INFINITE); }
	bool IsDying () const { return _isDying != 0; }
//protected:
    virtual ~ActiveObject () {}
	Thread::Handle StartThread ()
	{
		_thread.Resume ();
		return _thread;
	}
protected:
    virtual void Run () = 0;
	// Cut communication with external objects
	virtual void Detach () {}
	// Reset any events the thread could be waiting for
    virtual void FlushThread () = 0;
private:
	void WaitForKill () { _killEvent.Wait (); }
	// Note: ThreadEntry OWNS the ActiveObject
    static unsigned __stdcall ThreadEntry ( void *pArg);

protected:
	// Reads and writes to an int are guaranteed to be atomic
    int					_isDying;
	Thread::AutoHandle	_thread;
	Win::Event			_killEvent;
};

// Active object holder. Starts the thread.
// Kills the thread (which deletes the active object)

// Note: We cannot simply call Kill() from the active object's virtual destructor
// because Kill() calls the virtual function FlushThread()
// Never call the object's virtual functions from inside a destructor!

template<class T>
class auto_active
{
public:
	auto_active (T * active = 0) 
		: _active (active), _timeout (0)
	{
		if (_active)
			_thread = _active->StartThread ();
	}
	~auto_active ()
	{
		if (IsAlive () && _active != 0)
			_active->Kill (_timeout);
	}
	bool IsAlive () const { return !_thread.IsNull () && _thread.IsAlive (); }
	bool empty () const { return _active == 0; }
	void reset (T * active = 0)
	{
		if (IsAlive () && _active != 0)
			_active->Kill (_timeout);
		_active = active;
		if (_active)
			_thread = _active->StartThread ();
	}
	T * get () { return _active; }
	T & operator * () { return *_active; }
	T const & operator * () const { return *_active; }
	T * operator-> () { return _active; }
	// Normally auto_active destructor returns immediately
	// but you can tell it to wait for the actual death of thread
	void SetWaitForDeath (unsigned timeoutMs = INFINITE)
	{
		_timeout = timeoutMs;
	}
private:
	auto_active (auto_active const &);
	auto_active & operator= (auto_active const &);
private:
	T *				_active; // owned by ThreadEntry
	Thread::Handle	_thread;
	unsigned		_timeout;
};

#endif
