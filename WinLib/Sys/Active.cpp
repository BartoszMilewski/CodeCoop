#include <WinLibBase.h>
#include "Active.h"
#include "Ex/WinEx.h"
//----------------------------------
//  (c) Reliable Software, 1996-2005
//----------------------------------

//	ActiveObjects must always be allocated with new (never embedded or on the stack).
ActiveObject::ActiveObject ()
#pragma warning(disable: 4355) // 'this' used before initialized
    :_thread (ThreadEntry, this)
#pragma warning(default: 4355)
{
}

ActiveObject::~ActiveObject ()
{}

Thread::Handle ActiveObject::StartThread ()
{
	// ThreadEntry is the second owner
	Acquire (); // do this now, to avoid a race condition
	_thread.Resume ();
	return _thread;
}

// Called by outside thread
// FlushThread must reset all the events
// on which the thread might be waiting.
// return true of thread is dead
bool ActiveObject::Kill (unsigned timeoutMs) throw ()
{
	Detach (); // disconnect it from external objects
	_isDying.Inc ();
    FlushThread ();
    return _thread.WaitForDeath (timeoutMs);
}

// Executed by the worker thread

unsigned __stdcall ActiveObject::ThreadEntry (void* pArg)
{
	try
	{
		// Active has already been ref-conted for us (see StartThread)
		Win::SharedByTwoOwner<ActiveObject> active (reinterpret_cast<ActiveObject *> (pArg));
		active->Run ();
		// active is unreferenced here after the worker thread is done
	}
	catch (Win::Exception & e)
	{
		e;	//	prevent warning C4101 by referencing variable
#if defined _DEBUG
		::MessageBox (0, e.GetMessage (), "Terminating Thread", MB_ICONERROR | MB_OK);
#endif
	}
	catch (...)
	{
#if defined _DEBUG
		::MessageBox (0, "Unknown exception", "Terminating Thread", MB_ICONERROR | MB_OK);
#endif
	}
    return 0;
}

namespace UnitTest
{
	enum Status { DIED, TIMEDOUT, SIGNALED, INVALID };
	class Tester: public ActiveObject
	{
	public:
		Tester () : _status (INVALID) {}
		void Signal () { _event.Release ();	}
		Status GetStatus () const { return _status; }
		void Run ();
		void FlushThread () { _event.Release (); }
	private:
		Win::Event _event;
		Status _status;
	};

	void Tester::Run ()
	{
		bool success = _event.Wait (1000);
		if (IsDying ())
		{
			_status = DIED;
			return;
		}

		if (success)
			_status = SIGNALED;
		else
			_status = TIMEDOUT;
	}

	void ActiveObjectTest (std::ostream & out)
	{
		out << "Active object test" << std::endl;
		auto_active<Tester> tester (new Tester);
		tester.SetWaitForDeath ();

		out << "Polling for status (until suicide)" << std::endl;
		while (tester->GetStatus () == INVALID)
			continue;
		out << "done" << std::endl;
		::Sleep (100);
		
		out << "New tester: signaling" << std::endl;
		tester.reset (new Tester);
		tester->Signal ();
		::Sleep (100);

		out << "reseting the tester" << std::endl;
		tester.reset (new Tester);
		tester.reset ();
		out << "End active object test" << std::endl;
#if 0
#endif
	}
}
