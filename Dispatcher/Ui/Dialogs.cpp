//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include "precompiled.h"
#include "Dialogs.h"
#include "Addressee.h"
#include "ScriptInfo.h"
#include "UserIdPack.h"
#include "BuildOptions.h"
#include "Resource.h"

DeleteOrIgnoreCtrl::DeleteOrIgnoreCtrl (ScriptTicket const & script)
	: Dialog::ControlHandler (IDD_WHAT_WITH_SCRIPT),
	  _script (script)
{}

bool DeleteOrIgnoreCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_scriptInfoFrame.Init (dlgWin, IDC_SCRIPT_INFO_FRAME);
	_scriptName.Init (dlgWin, IDC_NAME);
	_project.Init (dlgWin, IDC_PROJECT);
	_senderHubId.Init (dlgWin, IDC_EMAIL);
	_senderId.Init (GetWindow (), IDC_LOCATION);
	_comment.Init (dlgWin, IDC_COMMENT);
	_recipients.Init (dlgWin, IDC_RECIPIENTS);

    _recipients.AddProportionalColumn (45, "Email");
    _recipients.AddProportionalColumn (20, "User Id");
    _recipients.AddProportionalColumn (33, "Status");

	std::string scriptInfoCaption ("Dispatcher is unable to process the following ");
	scriptInfoCaption += (_script.ToBeFwd () ? "outgoing" : "incoming");
	scriptInfoCaption += " script:  ";
	_scriptInfoFrame.SetText (scriptInfoCaption);
	_scriptName.SetString (_script.GetName ());
	_project.SetString (_script.GetProjectName ());
	_senderHubId.SetString (_script.GetSenderHubId ());
	UserIdPack pack (_script.GetSenderUserId ());
	_senderId.SetString (pack.GetUserIdString ());
	_comment.SetString (_script.GetComment ());

	for (int i = 0; i < _script.GetAddresseeCount (); i++)
	{
		ScriptInfo::RecipientInfo const & addressee = _script.GetAddressee (i);
		int idx = _recipients.AppendItem (addressee.GetHubId ().c_str ());
		UserIdPack pack (addressee.GetUserId ());
		_recipients.AddSubItem (pack.GetUserIdString (), idx, 1);
		std::string status;
		if (_script.IsStamped (i))
			status = "delivered";
		else if (_script.IsPassedOver (i))
			status = "not here";
		else if (_script.IsAddressingError (i))
			status = "unknown recipient";
		else
			status = "???";
		_recipients.AddSubItem (status, idx, 2);
	}
	dlgWin.SetForeground ();
	return true;
}

bool DeleteOrIgnoreCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}

BadScriptCtrl::BadScriptCtrl (BadScriptData * data)
	: Dialog::ControlHandler (IDD_BAD_SCRIPT),
	  _dlgData (data)
{}

bool BadScriptCtrl::OnInitDialog () throw (Win::Exception)
{
	_notice.Init (GetWindow (), IDC_NOTICE);
	_name.Init (GetWindow (), IDC_NAME);
	_path.Init (GetWindow (), IDC_PATH);

	std::string title ("Dispatcher: ");
	title += _dlgData->_title;
	GetWindow ().SetText (title.c_str ());

	_name.SetString (_dlgData->_name);
	_path.SetString (_dlgData->_path);
	_notice.SetText (_dlgData->_notice);

	GetWindow ().SetForeground ();
	return true;
}

bool BadScriptCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
