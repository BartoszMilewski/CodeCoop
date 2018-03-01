#if !defined (SYSHANDLE_H)
#define SYSHANDLE_H
// (c) Reliable Software 2003
#include "Handles.h"

namespace Sys
{
	// Sys::Handles share ::CloseHandle disposal policy
	typedef Win::Handle<HANDLE> Handle;

	template<class BaseHandle>
	struct Disposal
	{
		static void Dispose (BaseHandle h) throw () { ::CloseHandle (h.ToNative ()); }
	};

	typedef Win::AutoHandle<Sys::Handle, Disposal<Sys::Handle> > AutoHandle;
}
#endif
