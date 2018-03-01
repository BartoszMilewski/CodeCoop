//------------------------------------
//  (c) Reliable Software, 2002 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProjectInviteDlg.h"
#include "BrowseForFolder.h"
#include "Resource.h"

//
// General page handler
//

ProjectPageHandler::ProjectPageHandler (Project::InviteData & data, 
									  std::vector<std::string> const & remoteHubs,
									  std::string const & myEmail)
	: PropPage::Handler (IDD_PROJECT_INVITE),
	  _pageData (data),
	  _remoteHubs (remoteHubs),
	  _myEmail (myEmail)
{}

bool ProjectPageHandler::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());

	_projectName.Init (dlgWin, IDC_PROJECT_INVITE_PROJNAME);
	_userName.Init (dlgWin, IDC_PROJECT_INVITE_USERNAME);
	_peerOrHub.Init (dlgWin, IDC_PROJECT_INVITE_PEER_OR_HUB);
	_sat.Init (dlgWin, IDC_PROJECT_INVITE_SAT);
	_staticEmailAddr.Init (dlgWin, IDC_PROJECT_INVITE_STATIC_EMAIL);
	_emailAddress.Init (dlgWin, IDC_PROJECT_INVITE_EMAIL);
	_staticComputerName.Init (dlgWin, IDC_PROJECT_INVITE_STATIC_SAT);
	_computerName.Init (dlgWin, IDC_PROJECT_INVITE_SAT_NAME);

	_projectName.SetString (_pageData.GetProjectName ().c_str ());
	_peerOrHub.Check ();

	for (unsigned int i = 0; i < _remoteHubs.size (); ++i)
	{
		_emailAddress.AddToList (_remoteHubs [i]);
	}
	_emailAddress.AddToList (_myEmail);

	ResetSatControls (false); // disable

	return true;
}

bool ProjectPageHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_PROJECT_INVITE_PEER_OR_HUB:
		ResetSatControls (false); // disable
		break;
	case IDC_PROJECT_INVITE_SAT:
		ResetSatControls (true); // enable
		break;
	default:
		return false;
	}
    return true;
}

void ProjectPageHandler::OnKillActive (long & result) throw (Win::Exception)
{
	// Read page controls
	result = 0;	// Assume everything is ok
	_pageData.SetUserName (_userName.GetTrimmedString ());
	_pageData.SetEmailAddress (_emailAddress.RetrieveTrimmedEditText ());
	bool isSat = _sat.IsChecked ();
	_pageData.SetIsOnSatellite (isSat);
	if (isSat)
		_pageData.SetComputerName (_computerName.GetTrimmedString ());
	else
		_pageData.SetComputerName (std::string ());
}

void ProjectPageHandler::OnCancel (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	_pageData.Clear ();
}

void ProjectPageHandler::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok

	if (!_pageData.IsValid ())
	{
		_pageData.DisplayErrors (GetWindow ());
		result = 1;	// Don't close dialog
	}
}

void ProjectPageHandler::ResetSatControls (bool enable)
{
	if (enable)
	{
		_staticEmailAddr.SetText ("Hub's e-mail address:");
		_staticComputerName.Enable ();
		_computerName.Enable ();
	}
	else
	{
		_staticEmailAddr.SetText ("E-mail address:");
		_staticComputerName.Disable ();
		_computerName.Disable ();
	}
}

//
// Options page handler
//

OptionsPageHandler::OptionsPageHandler (Project::InviteData & data) 
	: PropPage::Handler (IDD_PROJECT_INVITE_OPTIONS),
	  _pageData (data)
{}

void OptionsPageHandler::OnKillActive (long & result) throw (Win::Exception)
{
	// Read page controls
	result = 0;	// Assume everything is ok
	_pageData.SetIsObserver (_observer.IsChecked ());
	_pageData.SetCheckoutNotification (_checkoutNotification.IsChecked ());
	if (_manualDispatch.IsChecked () || _transferHistory.IsChecked ())
	{
		_pageData.SetManualInvitationDispatch (_manualDispatch.IsChecked ());
		_pageData.SetTransferHistory (_transferHistory.IsChecked ());
		if (_myComputer.IsChecked ())
		{
			std::string path = _localFolder.GetString ();
			_pageData.SetLocalFolder (path);
			if (FilePath::IsNetwork (path))
				_pageData.SetLAN ();
			else
				_pageData.SetMyComputer ();
		}
		else if (_internet.IsChecked ())
		{
			_pageData.SetInternet ();
			Ftp::SmartLogin & ftpLogin = _pageData.GetFtpLogin ();
			ftpLogin.SetServer (_server.GetString ());
			ftpLogin.SetFolder (_serverFolder.GetString ());
			if (_anonymousLogin.IsChecked ())
			{
				ftpLogin.SetUser ("");
				ftpLogin.SetPassword ("");
			}
			else
			{
				ftpLogin.SetUser (_user.GetString ());
				ftpLogin.SetPassword (_password.GetString ());
			}
		}
	}
}

void OptionsPageHandler::OnApply (long & result) throw (Win::Exception)
{
	// Validation already done by project page
	result = 0;	// Assume everything is ok
}

void OptionsPageHandler::OnCancel (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	_pageData.Clear ();
}

bool OptionsPageHandler::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_observer.Init (dlgWin, IDC_PROJECT_INVITE_OBSERVER);
	_checkoutNotification.Init (dlgWin, IDC_START_CHECKOUT_NOTIFICATIONS);
	_manualDispatch.Init (dlgWin, IDC_SAVE_INVITATION);
	_transferHistory.Init (dlgWin, IDC_SAVE_HISTORY);
	_myComputer.Init (dlgWin, IDC_MY_COMPUTER);
	_localFolder.Init (dlgWin, IDC_LOCAL_FOLDER);
	_browseMyComputer.Init (dlgWin, IDC_MY_COMPUTER_BROWSE);
	_internet.Init (dlgWin, IDC_INTERNET);
	_server.Init (dlgWin, IDC_FTP_SERVER);
	_serverFolder.Init (dlgWin, IDC_FTP_FOLDER);
	_anonymousLogin.Init (dlgWin, IDC_ANONYMOUS_LOGIN);
	_user.Init (dlgWin, IDC_FTP_USER);
	_password.Init (dlgWin, IDC_FTP_PASSWORD);

	if (_pageData.IsStoreOnInternet ())
	{
		_internet.Check ();
	}
	else if (_pageData.IsStoreOnLAN () || _pageData.IsStoreOnMyComputer ())
	{
		_myComputer.Check ();
	}

	Ftp::SmartLogin const & ftpLogin = _pageData.GetFtpLogin ();
	_server.SetString (ftpLogin.GetServer ());
	_serverFolder.SetString (ftpLogin.GetFolder ());
	if (ftpLogin.IsAnonymous ())
	{
		_anonymousLogin.Check ();
		_user.Disable ();
		_password.Disable ();
	}
	else
	{
		_anonymousLogin.UnCheck ();
		_user.SetString (ftpLogin.GetUser ());
		_password.SetString (ftpLogin.GetPassword ());
	}
	_localFolder.SetString (_pageData.GetLocalFolder ());

	_internet.Disable ();
	_server.Disable ();
	_serverFolder.Disable ();
	_anonymousLogin.Disable ();
	_user.Disable ();
	_password.Disable ();
	_myComputer.Disable ();
	_browseMyComputer.Disable ();
	_localFolder.Disable ();

	return true;
}

bool OptionsPageHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_SAVE_INVITATION:
	case IDC_SAVE_HISTORY:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_manualDispatch.IsChecked () || _transferHistory.IsChecked ())
			{
				_internet.Enable ();
				_server.Enable ();
				_serverFolder.Enable ();
				_anonymousLogin.Enable ();
				_user.Enable ();
				_password.Enable ();
				_myComputer.Enable ();
				_browseMyComputer.Enable ();
				_localFolder.Enable ();
			}
			else
			{
				_internet.Disable ();
				_server.Disable ();
				_serverFolder.Disable ();
				_anonymousLogin.Disable ();
				_user.Disable ();
				_password.Disable ();
				_myComputer.Disable ();
				_browseMyComputer.Disable ();
				_localFolder.Disable ();
			}
		}
		return true;

	case IDC_MY_COMPUTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_localFolder.SetFocus ();
		}
		return true;

	case IDC_INTERNET:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_server.SetFocus ();
		}
		return true;

	case IDC_LOCAL_FOLDER:
		if (Win::Edit::GotFocus (notifyCode))
		{
			_myComputer.Check ();
			_internet.UnCheck ();
		}
		return true;

	case IDC_FTP_SERVER:
	case IDC_FTP_FOLDER:
		if (Win::Edit::GotFocus (notifyCode))
		{
			_myComputer.UnCheck ();
			_internet.Check ();
		}
		return true;

	case IDC_MY_COMPUTER_BROWSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_myComputer.Check ();
			_internet.UnCheck ();
			std::string path;
			std::string caption ("Select folder for the project '");
			caption += _pageData.GetProjectName ();
			caption += "' ";
			if (_manualDispatch.IsChecked () && _transferHistory.IsChecked ())
				caption += "invitation and history.";
			else if (_manualDispatch.IsChecked ())
				caption += "invitation.";
			else
				caption += "history.";

			if (BrowseForAnyFolder (path, GetWindow (), caption))
			{
				_localFolder.SetString (path);
			}
		}
		return true;

	case IDC_ANONYMOUS_LOGIN:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_anonymousLogin.IsChecked ())
			{
				_user.Disable ();
				_password.Disable ();
			}
			else
			{
				_user.Enable ();
				_password.Enable ();
			}
		}
		return true;
	}
	return false;
}

//
// Project invite handler set
//

ProjectInviteHandlerSet::ProjectInviteHandlerSet (Project::InviteData & data, 
												  std::vector<std::string> const & remoteHubs,
												  std::string const & myEmail)
	: PropPage::HandlerSet ("Invite New Project Member"),
	  _invitationData (data),
	  _projectPageHandler (_invitationData, remoteHubs, myEmail),
	  _optionsPageHandler (_invitationData)
{
	AddHandler (_projectPageHandler, "Project");
	AddHandler (_optionsPageHandler, "Options");
}

// -Project_Invite user:"invitee name" email:"invitee email" [observer:"yes"] [checkoutnotify:"yes"]
//                 [manualdispatch:"yes"] [transferhistory:"yes"] [satellite:"yes" computer:"name"]
//                 [target:"<invitation folder>"] [server:"<ftp server>"] [ftpuser:"<ftp user>"]
//				   [password:"<ftp password>"]
bool ProjectInviteHandlerSet::GetDataFrom (NamedValues const & source)
{
	std::string const userName = source.GetValue ("user");
	std::string const emailAddr = source.GetValue ("email");
	bool const isObserver = source.GetValue ("observer") == "yes";
	bool const isCheckoutNotification = source.GetValue ("checkoutnotify") == "yes";
	bool const isOnSat = source.GetValue ("satellite") == "yes";
	bool const isManualDispatch = source.GetValue ("manualdispatch") == "yes";
	bool const isTransferHistory = source.GetValue ("transferhistory") == "yes";
	std::string const computerName = source.GetValue ("computer");

	_invitationData.SetUserName (userName);
	_invitationData.SetEmailAddress (emailAddr);
	_invitationData.SetIsOnSatellite (isOnSat);
	if (isOnSat)
		_invitationData.SetComputerName (computerName);
	else
		_invitationData.SetComputerName (std::string ());
	_invitationData.SetIsObserver (isObserver);
	_invitationData.SetCheckoutNotification (isCheckoutNotification);
	_invitationData.SetManualInvitationDispatch (isManualDispatch);

	if (isManualDispatch || isTransferHistory)
	{
		std::string targetPath (source.GetValue ("target"));
		std::string server (source.GetValue ("server"));
		if (!server.empty ())
		{
			_invitationData.SetInternet ();
			Ftp::SmartLogin & ftpLogin = _invitationData.GetFtpLogin ();
			ftpLogin.SetServer (server);
			ftpLogin.SetFolder (targetPath);
			ftpLogin.SetUser (source.GetValue ("user"));
			ftpLogin.SetPassword (source.GetValue ("password"));
		}
		else
		{
			_invitationData.SetLocalFolder (targetPath);
			if (FilePath::IsNetwork (targetPath))
				_invitationData.SetLAN ();
			else
				_invitationData.SetMyComputer ();
		}
	}
	return _invitationData.IsValid ();
}
