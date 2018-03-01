//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "Commander.h"
#include "Ctrl.h"
#include "View.h"
#include <Dbg/Assert.h>

void Commander::Program_Exit () 
{
	_ctrl.GetWindow ().Destroy ();
}

void Commander::Directory_Up ()
{
	_ctrl.DirUp ();
}

Cmd::Status Commander::can_Directory_Up () const 
{
	return _ctrl.IsDown ()? Cmd::Enabled: Cmd::Disabled;
}

void Commander::View_Old ()
{
	_ctrl.StartSessionOld ();
}

void Commander::View_New ()
{
	_ctrl.StartSessionNew ();
}

void Commander::View_Diff ()
{
	_ctrl.StartSessionDiff ();
}

void Commander::Selection_View ()
{
	Assert (_view != 0);
	int i = _view->GetFirstSelected ();
	if (i != -1)
	{
		_ctrl.Open (i);
	}
}

Cmd::Status Commander::can_Selection_View () const 
{
	if (_view != 0 && _view->IsSelection ())
		return Cmd::Enabled;
	else
		return Cmd::Disabled;
}

void Commander::Directory_Refresh ()
{
	_ctrl.Refresh ();
}

