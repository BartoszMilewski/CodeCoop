#if !defined ACTIVE_H
#define ACTIVE_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2007
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

#include <Sys/Thread.h>
#include <Sys/Synchro.h>

class ActiveObject: public Win::SharedByTwo
{
	template<class T>
	friend class auto_active;
public:
    ActiveObject ();
	// Note: Do not call Kill() from the active object's virtual destructor
	// because Kill() calls the virtual function FlushThread ()
	// Never call the object's virtual functions from inside a destructor!
	virtual ~ActiveObject ();
	bool IsDying () const { return _isDying.IsNonZero (); }
	// Kill will stop the thread and cause the eventual
	// (asynchronous) deletion of the ActiveObject
    bool Kill (unsigned timeoutMs = 0) throw ();
	void KillSynchronous () throw () { Kill (INFINITE); }
	void Nuke () { _thread.Kill (); }
protected:
	Thread::Handle StartThread ();
protected:
    virtual void Run () = 0;
	// Must cut communication with external objects
	virtual void Detach () {}
	// Must reset any events the thread could be waiting for
    virtual void FlushThread () = 0;
private:
	// Note: ThreadEntry OWNS one reference to the ActiveObject
    static unsigned __stdcall ThreadEntry ( void *pArg);
protected:
	Win::AtomicCounter	_isDying;
	Thread::AutoHandle	_thread;
};

// Active object holder. Starts the thread.
// Kills the thread, when reset() or in its destructor

// Note: We cannot simply call Kill() from the active object's virtual destructor
// because Kill() calls the virtual function FlushThread ()
// Never call the object's virtual functions from inside a destructor!

template<class T>
class auto_active
{
public:
	auto_active (T * active = 0) 
		: _active (active), _timeout (0)
	{
		if (!_active.empty ())
		{
			_thread = _active->StartThread ();
		}
	}
	~auto_active ()
	{
		if (IsAlive () && !_active.empty ())
		{
			_active->Kill (_timeout);
		}
	}
	void Nuke () 
	{ 
		if (!_active.empty () && IsAlive ())
			_active->Nuke ();
		_active = 0;
		_thread = Thread::Handle ();
	}
	bool IsAlive () const { return !_thread.IsNull () && _thread.IsAlive (); }
	bool empty () const { return _active.empty (); }
	void reset (T * active)
	{
		if (!_active.empty () && IsAlive ())
		{
			_active->Kill (_timeout);
		}
		_active = active;
		Assert (active != 0);
		_thread = _active->StartThread ();
	}
	// Okay to call IsAlive after reset
	void reset ()
	{
		if (!_active.empty () && IsAlive ())
		{
			_active->Kill (_timeout);
		}
		_active = 0;
		// _thread = Thread::Handle ();
	}
	T * get () { return _active.GetPtr<T> (); }
	T & operator * () { return *_active.GetPtr<T> (); }
	T const & operator * () const { return *_active.GetPtr<T>(); }
	T * operator-> () { return _active.GetPtr<T>(); }
	T const * operator-> () const { return _active.GetPtr<T> (); }
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
	Win::SharedByTwoOwner<T>	_active; // owned also by ThreadEntry
	Thread::Handle		_thread;
	unsigned			_timeout;
};

#endif
