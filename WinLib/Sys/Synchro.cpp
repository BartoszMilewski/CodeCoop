//------------------------------
// (c) Reliable Software 2000-03
//------------------------------
#include <WinLibBase.h>
#include "Synchro.h"
#include "GlobalUniqueName.h"

/* Disable "The function 'InitializeCriticalSection' must be called from within a try/except block" */
#pragma warning(disable:28125)

namespace Win
{
    CritSection::CritSection()
        : _count(0)
    {
        ::InitializeCriticalSection(&_critSection);
    }

    CritSection::~CritSection()
    {
        ::DeleteCriticalSection(&_critSection);
    }
    
    void CritSection::Init()
    {
        Assert(_count == 0xffffffff);
        _count = 0;
        ::InitializeCriticalSection(&_critSection);
    }

	NewEvent::NewEvent ()
		: Event (0)
	{
		GlobalUniqueName uname;
		_name = uname.GetName ();
		BaseHRef () = ::CreateEvent (0, false, false, _name.c_str ());
		if (IsNull ())
			throw Win::Exception ("Cannot create named event");
	}

#if 0	//	CreateWaitableTimer not available on Win95
	WaitableTimer::WaitableTimer ()
	{
		GlobalUniqueName uname;
		_name = uname.GetName ();
		_h = ::CreateWaitableTimer (0, TRUE, _name.c_str ());
		DWORD lastErr = ::GetLastError ();
		if (lastErr == ERROR_ALREADY_EXISTS)
			throw Win::Exception ("Internal error: Cannot create waitable timer -- timer with the same name already exists.", _name.c_str ());
		if (lastErr == ERROR_INVALID_HANDLE)
			throw Win::Exception ("Internal error: Cannot create waitable timer -- timer name already used by other kernel object.", _name.c_str ());
		if (IsNull ())
			throw Win::Exception ("Internal error: Cannot create waitable timer.", _name.c_str ());
	}

	WaitableTimer::WaitableTimer (std::string const & name)
		: Sys::AutoHandle (::OpenWaitableTimer (TIMER_ALL_ACCESS, TRUE, name.c_str ()))
	{
		if (IsNull ())
			throw Win::Exception ("Internal error: Cannot open waitable timer.", name.c_str ());
		_name = name;
	}

	void WaitableTimer::Set (unsigned int timeout)
	{
		Assert (!IsNull ());
		// Time interval has to be expressed in 100 nanoseconds
		// Convert miliseconds to nanoseconds
		// 1 milisecond = 1 000 000 nanoseconds or 10 000 * 100 nanoseconds
		long long tmp = 10000 * timeout;
		// Positive values indicate absolute time.
		// Negative values indicate relative time.
		tmp = -tmp;
		LARGE_INTEGER dueTime;
		dueTime.QuadPart = tmp;
		if (::SetWaitableTimer (_h, &dueTime, 0, 0, 0, FALSE) == 0)
			throw Win::Exception ("Internal error: Cannot set waitable timer.", _name.c_str ());
	}

	bool WaitableTimer::Wait ()
	{
		Assert (!IsNull ());
        DWORD waitStatus = ::WaitForSingleObject (_h, INFINITE);
		return waitStatus == WAIT_OBJECT_0;
	}
#endif

}