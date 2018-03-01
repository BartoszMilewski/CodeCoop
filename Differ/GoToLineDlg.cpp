//------------------------------------
//  (c) Reliable Software, 2001 - 2005
//------------------------------------

#include "precompiled.h"
#include "GoToLineDlg.h"
#include "EditParams.h"
#include "resource.h"

#include <Win/Message.h>
#include <Win/Dialog.h>

GoToLineDlgController::GoToLineDlgController (GoToLineDlgData & dlgData)
	: Dialog::ControlHandler (IDD_GOTOLINE),
	  _dlgData (dlgData)
{}


bool GoToLineDlgController::OnInitDialog () throw (Win::Exception)
{
	// init edit box
	_paraNo.Init (GetWindow (), IDC_GOTOLINE_NUMBER_LINE);
	std::string stringParaNo = ToString (_dlgData.GetParaNo ());
	_paraNo.SetString (stringParaNo.c_str ());
	
	// init static box
	_lastParaNo.Init (GetWindow (), IDC_LINE_RANGE);
	std::string lineRange ("Line numbers: 1 - ");
	lineRange += ToString (_dlgData.GetLastParaNo ());
	_lastParaNo.SetString (lineRange.c_str ());

	// center dialog
	Dialog::Handle  dlg (GetWindow ());
	dlg.CenterOverOwner ();
	return true;
}

bool GoToLineDlgController::OnApply () throw ()
{ 
	std::string paraNoS = _paraNo.GetString ();
	int paraNo = atoi (paraNoS.c_str ()) - 1;
	_dlgData.SetParaNo (paraNo);				
	EndOk ();
	return true;
}
