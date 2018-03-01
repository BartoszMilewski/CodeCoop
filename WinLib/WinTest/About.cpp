//----------------------------
// (c) Reliable Software, 2005
//----------------------------

#include "precompiled.h"
#include "About.h"

#include <Com/Shell.h>
#include <Ctrl/Controls.h>

// Note: Strictly speaking, in this particular case, 
// OnInitDialog doesn't have to be overridden
// because the default OnInitDialog does exactly the same.
bool AboutDlgHandler::OnInitDialog () throw (Win::Exception) 
{
	return true;
}

bool AboutDlgHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDB_RELISOFT:
		ShellMan::Open (GetWindow (), "http://www.relisoft.com");
		return true;
	}
    return false;
}

// Note: Strictly speaking, this method doesn't have to be overridden
// It has the correct default implementation.
bool AboutDlgHandler::OnApply () throw ()
{
	EndOk ();
	return true;
}
