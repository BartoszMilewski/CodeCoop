// ----------------------------------
// (c) Reliable Software, 2005 - 2007
// ----------------------------------

#include "precompiled.h"

#include "AboutDlg.h"
#include "BuildOptions.h"
#include "resource.h"

#include <Ctrl/Static.h>

AboutCtrl::AboutCtrl ()
	: Dialog::ControlHandler (IDD_ABOUT)
{}

bool AboutCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::StaticText stc (GetWindow (), IDC_ABOUT);
	stc.SetText (DISPATCHER_PRODUCT_NAME " v. " DISPATCHER_FILE_VERSION " by Reliable Software");

	Win::StaticText	copyright (GetWindow (), IDC_COPYRIGHT);
	copyright.SetText (COPYRIGHT);

	return true;
}

bool AboutCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
