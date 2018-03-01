#if !defined THREAD_H
#define THREAD_H
//------------------------------------
//  (c) Reliable Software, 1996-2003
//------------------------------------
#include <Win/SysHandle.h>
#include <process.h>

namespace Thread
{
	inline unsigned long GetCurrentId ()
	{
		return ::GetCurrentThreadId ();
	}

	class Handle: public Sys::Handle
	{
	public:
		Handle (HANDLE h = Sys::Handle::NullValue ())
			: Sys::Handle (h)
		{}
    	void Resume () 
		{ 
			::ResumeThread (H ()); 
		}
    	void Suspend () 
		{ 
			::SuspendThread (H ()); 
		}
		// true if dead
	    bool WaitForDeath (unsigned timeoutMs = INFINITE)
    	{
        	return ::WaitForSingleObject (H (), timeoutMs) == WAIT_OBJECT_0;
    	}
		bool IsAlive () const
		{
			unsigned long code;
			::GetExitCodeThread (H (), &code);
			return code == STILL_ACTIVE;
		}
	};

	typedef Win::AutoHandle<Thread::Handle, Sys::Disposal<Thread::Handle> > auto_handle;

	class AutoHandle: public auto_handle
	{
	public:
    	AutoHandle ( unsigned (__stdcall * pFun) (void* arg), void* pArg)
			: auto_handle (
				reinterpret_cast<HANDLE> (_beginthreadex (
        	    0, // Security attributes
            	0, // Stack size
	            pFun, 
    	        pArg, 
        	    CREATE_SUSPENDED, 
            	&_tid)))
		{
			if (IsNull ())
				throw Win::Exception ("Cannot create thread");
    	}
		void Kill (int status = 0)
		{
			_endthreadex (status);
		}
	private:
    	unsigned  _tid;     // thread id
	};
}
#endif
