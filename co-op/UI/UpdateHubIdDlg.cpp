//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "UpdateHubIdDlg.h"
#include "Resource.h"

UpdateHubIdCtrl::UpdateHubIdCtrl (std::string const & catalogHubId, std::string const & userAddress)
	: Dialog::ControlHandler (IDD_UPDATE_HUB_ID),
	  _catalogHubId (catalogHubId),
	  _userAddress (userAddress)
{}

bool UpdateHubIdCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_hubId.Init (dlgWin, IDC_CATALOG_HUB_ID);
	_address.Init (dlgWin, IDC_USER_ADDRESS);

	_hubId.SetText (_catalogHubId.c_str ());
	_address.SetText (_userAddress.c_str ());
	return true;
}

bool UpdateHubIdCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

