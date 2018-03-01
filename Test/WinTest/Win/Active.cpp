#include "Active.h"
#include "Win/WinEx.h"
#include <memory>
//------------------------------------
//  (c) Reliable Software, 1996, 2003
//------------------------------------


ActiveObject::ActiveObject ()
: _isDying (0),
#pragma warning(disable: 4355) // 'this' used before initialized
  _thread (ThreadEntry, this)
#pragma warning(default: 4355)
{
}

// Called by outside thread
// FlushThread must reset all the events
// on which the thread might be waiting.

void ActiveObject::Kill (unsigned timeoutMs) throw ()
{
	Detach (); // disconnect it from external objects
    _isDying = 1;
    FlushThread ();
	_killEvent.Release ();
    _thread.WaitForDeath (timeoutMs);
}

// Executed by the worker thread

unsigned __stdcall ActiveObject::ThreadEntry (void* pArg)
{
	try
	{
		std::auto_ptr<ActiveObject> active (reinterpret_cast<ActiveObject *> (pArg));
		active->Run ();
		active->WaitForKill ();
		// active is deleted here after the worker thread is done
	}
	catch (Win::Exception & e)
	{
		e;	//	prevent warning C4101 by referencing variable
#if !defined NDEBUG
		::MessageBox (0, e.GetMessage (), "Terminating Thread", MB_ICONERROR | MB_OK);
#endif
	}
	catch (...)
	{
#if !defined NDEBUG
		::MessageBox (0, "Unknown exception", "Terminating Thread", MB_ICONERROR | MB_OK);
#endif
	}
    return 0;
}
