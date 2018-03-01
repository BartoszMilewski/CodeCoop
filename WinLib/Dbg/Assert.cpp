//-----------------------------------------------
//
//  (c) Reliable Software, 2002 -- 2002
//-----------------------------------------------

#include <WinLibBase.h>
#if defined (_DEBUG)

#include "Assert.h"
#include <stdlib.h>

namespace Dbg
{
	class MainThreadInfo
	{
	public:
		MainThreadInfo ()
		: _tid (::GetCurrentThreadId ())
		{}
		DWORD Id () { return _tid; }
		
	private:
		DWORD const _tid;
	};
	
	//	must be a static instance so it gets constructed on the main thread
	MainThreadInfo mainThreadInfo;
	
	bool IsMainThread ()
	{
		return ::GetCurrentThreadId () == mainThreadInfo.Id ();
	}

	bool OutputFutureAssert (char const *file, int line, char const *expr)
	{
		//	can't use dbg output object (because it allocates memory)
		::OutputDebugString ("*** Future Assert: (");
		::OutputDebugString (file);
		::OutputDebugString (", line ");
		char lineBuf [16];
		::OutputDebugString (::_itoa (line, lineBuf, 10));
		::OutputDebugString (") ");
		::OutputDebugString (expr);
		::OutputDebugString ("\n");
		return false;
	}
}

#endif
