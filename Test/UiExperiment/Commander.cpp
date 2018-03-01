//----------------------------------
// (c) Reliable Software 2007-09
//----------------------------------

#include "precompiled.h"
#include "Commander.h"


Commander::Commander (Win::Dow::Handle hwnd)
	: _hwnd (hwnd)
{}

void Commander::Browser_Exit ()
{
	throw Win::ExitException (0);
}
