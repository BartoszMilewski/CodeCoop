//------------------------------------
// (c) Reliable Software 1997
//------------------------------------
#include "WinEx.h"

using namespace Win;

Exception::Exception (char const * msg, char const * objName)
    : _msg (msg), 
	  _hModule (0)
{
	_err = ::GetLastError ();
	::SetLastError (0);
	InitObjName (objName);
}

Exception::Exception (char const * msg, char const * objName, DWORD err, HINSTANCE hModule)
    : _msg (msg), 
	  _err (err),
	  _hModule (hModule)
{
	::SetLastError (0);
	InitObjName (objName);
}

void Exception::InitObjName (char const * objName)
{
	if (objName != 0)
	{
		strncpy (_objName, objName, sizeof (_objName) - 1);
		_objName [sizeof (_objName) - 1] = '\0';
	}
	else
		_objName [0] = '\0';
}
