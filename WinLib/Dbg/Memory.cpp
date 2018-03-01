//-----------------------------------------------
//
//  (c) Reliable Software, 2002 -- 2002
//-----------------------------------------------

#include <WinLibBase.h>

#if defined (_DEBUG)

#include "Memory.h"
#include "Out.h"

namespace Dbg
{
	Win::Mutex NoMemoryAllocations::_mutex;
	std::list<DWORD> NoMemoryAllocations::_tidList;
	std::list<DWORD>::size_type NoMemoryAllocations::_tidListSize;
	_CRT_ALLOC_HOOK volatile NoMemoryAllocations::_oldHook;

	NoMemoryAllocations::NoMemoryAllocations ()
	{
		Win::MutexLock lock (_mutex);

		//	No nesting please!  Nesting could be supported if we added a
		//	counter per thread id.
		Assert (std::find (_tidList.begin (), _tidList.end (), ::GetCurrentThreadId ()) == _tidList.end ());

		//	find a place to store this thread id (reusing a spot if we can)
		std::list<DWORD>::iterator it = std::find (_tidList.begin (), _tidList.end (), -1);

		if (it == _tidList.end ())
		{
			_tidList.push_back (::GetCurrentThreadId ());
			++_tidListSize;
			Assert (_tidListSize == _tidList.size ());
		}
		else
		{
			*it = ::GetCurrentThreadId ();
		}

		if (_oldHook == 0)
		{
			_oldHook = _CrtSetAllocHook (AllocHook);
		}
	}

	NoMemoryAllocations::~NoMemoryAllocations ()
	{
		Win::MutexLock lock (_mutex);

		std::list<DWORD>::iterator it = std::find (_tidList.begin (), _tidList.end (), ::GetCurrentThreadId ());
		Assert (it != _tidList.end ());

		*it = -1;

		for (std::list<DWORD>::iterator it = _tidList.begin (); it != _tidList.end (); ++it)
		{
			if (*it != -1)
			{
				//	some other thread still using the hook
				return;
			}
		}

		Assert (_oldHook != 0);
		_CrtSetAllocHook (_oldHook);
		_oldHook = 0;
	}

	int NoMemoryAllocations::AllocHook (int allocType, void *userData, size_t size, int blockType, 
										long requestNumber, const unsigned char *filename, int lineNumber)
	{
		//	Can't take the mutex in this function (because we've already taken
		//	the heap mutex)

		bool illegalAlloc = false;

		if (allocType == _HOOK_ALLOC || allocType == _HOOK_REALLOC)
		{
			std::list<DWORD>::iterator it = _tidList.begin ();
			for (std::list<DWORD>::size_type i = 0; i < _tidListSize; ++i, ++it)
			{
				if (*it == ::GetCurrentThreadId ())
				{
					illegalAlloc = true;
					break;
				}
			}
		}

		if (illegalAlloc)
		{
			FutureAssert (0 && "Memory allocation in NoMemoryAllocations scope");
			//	return false;	//	Too aggressive for the moment
		}

		//	Since _oldHook might change any time, we take a local copy that we can test and then use
		//	Also, _oldHook might not have been set by the time the new hook was installed
		_CRT_ALLOC_HOOK oldHook = _oldHook;
		if (oldHook != 0)
		{
			return (*oldHook) (allocType, userData, size, blockType, requestNumber, filename, lineNumber);
		}

		return true;
	}
}

#endif
