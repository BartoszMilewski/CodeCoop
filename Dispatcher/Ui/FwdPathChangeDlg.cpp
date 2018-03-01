// ----------------------------------
// (c) Reliable Software, 2001 - 2006
// ----------------------------------

#include "precompiled.h"
#include "FwdPathChangeDlg.h"
#include "UserIdPack.h"
#include "Config.h"
#include "BrowseForFolder.h"
#include "Resource.h"

FwdPathChangeCtrl::FwdPathChangeCtrl (FwdPathChangeData & data)
	: Dialog::ControlHandler (IDD_FWD_PATH_CHANGE),
	  _dlgData (data)
{}

bool FwdPathChangeCtrl::OnInitDialog () throw (Win::Exception)
{ 
	_hubIdEdit.Init (GetWindow (), IDC_EMAIL);
	_projEdit.Init (GetWindow (), IDC_PROJECT);
	_locEdit.Init (GetWindow (), IDC_LOCATION);
	_pathEdit.Init (GetWindow (), IDC_PATH);
	_browse.Init (GetWindow (), IDB_BROWSE);
	_replaceAll.Init (GetWindow (), IDC_REPLACE_ALL);

	Address const & address = _dlgData.GetAddress ();
    _hubIdEdit.SetString (address.GetHubId ().c_str ());
    _projEdit.SetString  (address.GetProjectName ().c_str ());
	UserIdPack userId (address.GetUserId ());
    _locEdit.SetString   (userId.GetUserIdString ());
	_pathEdit.SetString  (_dlgData.GetNewTransport ().GetRoute ().c_str ());
	
	if (!_dlgData.IsToReplaceAll ())
		_replaceAll.Disable ();

	return true;
}

bool FwdPathChangeCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if  (ctrlId == IDB_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string folder;
		if (BrowseForSatelliteShare (folder, GetWindow ()))
			_pathEdit.SetString (folder);
		return true;
	};
    return false;
}

bool FwdPathChangeCtrl::OnApply () throw ()
{
	// revisit: let user specify method???
	Transport transport (_pathEdit.GetString ());
	bool validTransport = TheTransportValidator->ValidateExcludePublicInbox (
																	transport,
																	"You cannot specify your own Public Inbox\n"
																	"as the forwarding path to your satellite.",
																	GetWindow ());
	if (validTransport)
	{
		if (_dlgData.IsToReplaceAll ())
		{
			_dlgData.SetReplaceAll (_replaceAll.IsChecked ());
		}
		_dlgData.SetNewTransport (transport);
		EndOk ();
	}
	return true;
}
