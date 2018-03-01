//------------------------------
// (c) Reliable Software 2000-03
//------------------------------

#include <WinLibBase.h>
#include "EnumProcess.h"
#include <StringOp.h>

using namespace Win;

EnumProcess::EnumProcess (Win::Dow::Handle callerWindow, Messenger & messenger)
	: _callerWindow (callerWindow),
	  _messenger (messenger)
{
	// Enumerate processes with the same class name as caller Window
	std::string buf;
	_callerWindow.GetClassName (buf);
	::EnumWindows (&EnumCallback, reinterpret_cast<LPARAM>(this));
}

EnumProcess::EnumProcess (std::string const & className, Messenger & messenger)
	: _callerWindow (0),
	  _className (className),
	  _messenger (messenger)
{
	// Enumerate processes with given class name
	::EnumWindows (&EnumCallback, reinterpret_cast<LPARAM>(this));
}

void EnumProcess::ProcessWindow (Win::Dow::Handle win)
{
	if (win != _callerWindow)
	{
		std::string buf;
		win.GetClassName (buf);
		if (_className == buf)
		{
			// Found another window of the same class -- execute command
			_messenger.DeliverMessage (win);
		}
	}
}

BOOL CALLBACK EnumProcess::EnumCallback (HWND hwnd, LPARAM lParam)
{
	Assert (lParam != 0);
	EnumProcess * thisEnum = reinterpret_cast<EnumProcess *>(lParam);
	thisEnum->ProcessWindow (hwnd);
	return TRUE;
}
