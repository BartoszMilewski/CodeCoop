// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "DestinationPage.h"
#include "HeaderDetails.h"
#include "resource.h"

#include <Com/Shell.h>

void DestinationHandler::OnKillActive (long & result) throw (Win::Exception)
{
	_ctrl.RetrieveData ();
	if (_ctrl.GetDetails ().IsDestDataValid ())
	{
		result = FALSE;
	}
	else
	{
		_ctrl.GetDetails ().DisplayDestErrors ();
		result = TRUE;
	}
}

void DestinationCtrl::OnInitDialog () throw (Win::Exception)
{
	_pi.Init (GetWindow (), IDC_PI);
	_inbox.Init (GetWindow (), IDC_INBOX);
	_outbox.Init (GetWindow (), IDC_OUTBOX);
	_project.Init (GetWindow (), IDC_PROJECT_COMBO);
	_userId.Init (GetWindow (), IDC_USERID);
	_other.Init (GetWindow (), IDC_OTHER);
	_path.Init (GetWindow (), IDC_PATH);
	_browse.Init (GetWindow (), IDC_BROWSE);
	_filename.Init (GetWindow (), IDC_FILENAME);

	_pi.Check ();
	_project.Disable ();
	_userId.Disable ();
	_path.Disable ();
	_browse.Disable ();

	_filename.SetText (_details._scriptFilename.c_str ());
}

bool DestinationCtrl::OnCommand (int ctrlId, int notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_PI:
		_project.Disable ();
		_userId.Disable ();
		_path.Disable ();
		_browse.Disable ();
		return true;
	case IDC_INBOX:
	case IDC_OUTBOX:
		_project.Enable ();
		_userId.Enable ();
		_path.Disable ();
		_browse.Disable ();
		return true;
	case IDC_OTHER:
		_project.Disable ();
		_userId.Disable ();
		_path.Enable ();
		_browse.Enable ();
		return true;
	case IDC_BROWSE:
		if (_browse.IsClicked (notifyCode))
		{
			ShellMan::VirtualDesktopFolder root;
			ShellMan::FolderBrowser folder (
								  GetWindow (),
								  root,
								  "Select the destination folder for the script");
			if (folder.IsOK ())
			{
				_path.SetString (folder.GetPath ());
			}
			return true;
		}
	};
	return false;
}

void DestinationCtrl::RetrieveData ()
{
	if (_pi.IsChecked ())
	{
		_details._destFolder = HeaderDetails::PublicInbox;
	}
	else if (_other.IsChecked ())
	{
		_details._destFolder = HeaderDetails::UserDefined;
		_details._destPath = _path.GetString ();
	}
	else
	{
		_details._destFolder  = _inbox.IsChecked () ? HeaderDetails::PrivateInbox : 
													  HeaderDetails::PrivateOutbox;
		_details._destProject = _project.RetrieveEditText ();
		_details._destUserId  = _userId.RetrieveEditText ();
	}
	_details._scriptFilename = _filename.GetString ();
}
