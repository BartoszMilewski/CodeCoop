//---------------------------
// (c) Reliable Software 2005
//---------------------------

#include "precompiled.h"
#include "TabDlg.h"
#include "Resource.h"
#include "OutputSink.h"

TabSizeCtrl::TabSizeCtrl (unsigned & tabSize)
	: Dialog::ControlHandler (IDD_TAB_SIZE),
	  _tabSize (tabSize)
{}

bool TabSizeCtrl::OnInitDialog () throw (Win::Exception)
{
	_edit.Init (GetWindow (), IDC_EDIT);
	_edit.SetText (ToString (_tabSize));
	return true;
}

bool TabSizeCtrl::OnApply () throw ()
{
	int newSize;
	_edit.GetInt (newSize);
	if (newSize <= 0 || newSize > 12)
	{
		TheOutput.Display ("Enter tab size > 0 and < 13");
	}
	else
	{
		_tabSize = newSize;
		EndOk ();
	}
	return true;
}

