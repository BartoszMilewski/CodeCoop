//----------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

#include "precompiled.h"
#include "FwdFolderDlg.h"
#include "Config.h"
#include "BrowseForFolder.h"
#include "OutputSink.h"
#include "resource.h"

FwdFolderCtrl::FwdFolderCtrl (FwdFolderData & data, bool isJoin)
	: Dialog::ControlHandler (isJoin ? IDD_FWD_FOLDER_JOIN : IDD_FWD_FOLDER_RECEIVE),
	  _dlgData (data)
{}

bool FwdFolderCtrl::OnInitDialog () throw (Win::Exception)
{
	_hubId.Init (GetWindow (), IDC_FWD_EMAIL);
	_userId.Init (GetWindow (), IDC_USER_ID);
	_name.Init (GetWindow (), IDC_FWD_PROJECT);
	_transport.Init (GetWindow (), IDC_FWD_PATH_EDIT);
	_browse.Init (GetWindow (), IDC_FWD_BROWSE);
	_comment.Init (GetWindow (), IDC_COMMENT);

	_hubId.SetString (_dlgData.GetHubId ().c_str ());
    _userId.SetString (_dlgData.GetUserId ().c_str ());
    _name.SetString (_dlgData.GetProjectName ().c_str ());
	// Revisit: show also transport method
	_transport.SetString (_dlgData.GetTransport ().GetRoute ());
	_comment.SetString (_dlgData.GetComment ().c_str ());
	// Make sure that dialog is visible
	GetWindow ().SetForeground ();
	return true;
}

bool FwdFolderCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_FWD_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string folder;
		if (BrowseForSatelliteShare (folder, GetWindow ()))
			_transport.SetString (folder);

		return true;
	}
    return false;
}

bool FwdFolderCtrl::OnApply () throw ()
{
	// revisit: guess method, but let user override?
	Transport transport (_transport.GetString ());
	bool validTransport = TheTransportValidator->ValidateExcludePublicInbox (
															transport,
															"You cannot specify your own Public Inbox\n"
															"as the forwarding path to your satellite.",
															GetWindow ());
	if (validTransport)
	{
		_dlgData.SetTransport (transport);
		EndOk ();
	}
	return true;
}

bool FwdFolderCtrl::OnCancel () throw ()
{
	_dlgData.SetTransport (Transport ("", Transport::Unknown));
	EndCancel ();
	return true;
}