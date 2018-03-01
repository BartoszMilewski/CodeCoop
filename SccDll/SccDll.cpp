//----------------------------------
// (c) Reliable Software 1999 - 2007
//----------------------------------

#include "precompiled.h"
#include "SccDll.h"
#include "Global.h"
#include "Catalog.h"

#include <Dbg/Out.h>
#include <Dbg/Log.h>

#include <Win/WinClass.h>
#include <Com/Shell.h>

#include <time.h>

Win::Instance DllHInstance;
CatSingleton TheCatSingleton;

Catalog & CatSingleton::GetCatalog ()
{
	Win::Lock lock (_critSection);
	if (_cat.get () == 0)
	{
		_cat.reset (new Catalog ());
	}
	return *_cat.get ();
}

BOOL APIENTRY DllMain (HINSTANCE hModule, DWORD context, LPVOID reserved)
{
	DllHInstance = hModule;
    switch (context)
	{
	case DLL_PROCESS_ATTACH:
		// Don't call anything serious from here!
		std::srand (static_cast<unsigned int>(time (0)));
		TheCatSingleton.Init ();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
    }
    return TRUE;
}