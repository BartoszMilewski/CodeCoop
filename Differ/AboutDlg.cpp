//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include "precompiled.h"
#include "AboutDlg.h"
#include "FileNames.h"
#include "Resource.h"
#include "BuildOptions.h"

HelpAboutData::HelpAboutData (FileNames const & fileNames)
{
}

HelpAboutCtrl::HelpAboutCtrl (HelpAboutData & data)
	: Dialog::ControlHandler (IDD_ABOUT),
	 _dlgData (data)
{}

bool HelpAboutCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::StaticText stc (GetWindow (), IDC_ABOUT);
	stc.SetText ("Differentiator, v. " DIFFER_FILE_VERSION);
	
	Win::StaticText copyright (GetWindow (), IDC_COPYRIGHT);
	copyright.SetText (COPYRIGHT);

	_msg.Init (GetWindow (), IDC_ABOUT_MSG);
	_msg.SetText (_dlgData.GetMsg ());
	return true;
}

bool HelpAboutCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
