//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "precompiled.h"
#include "JoinProject.h"
#include "Catalog.h"
#include "LocalRecipient.h"
#include "OutputSink.h"
#include "MemberDescription.h"
#include "HubRemoteConfig.h"
#include "AppHelp.h"
#include "BrowseForFolder.h"
#include "Prompter.h"

class RemoteHubAdvancedCtrl : public Dialog::ControlHandler
{
public:
	RemoteHubAdvancedCtrl (HubRemoteConfig & hubData)
		: Dialog::ControlHandler (IDD_REMOTE_HUB_TRANS),
		  _hubData (hubData)
	{}
	
    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::Edit			_hubId;
	Win::Edit			_transport;
	HubRemoteConfig &	_hubData;
};

bool RemoteHubAdvancedCtrl::OnInitDialog () throw (Win::Exception)
{
	_hubId.Init (GetWindow (), IDC_HUB_ID);
	_transport.Init (GetWindow (), IDC_HUB_REMOTE_TRANS);
	_hubId.SetString (_hubData.GetHubId ());
	_transport.SetString (_hubData.GetTransport ().GetRoute ().c_str ());
	return true;
}

bool RemoteHubAdvancedCtrl::OnApply () throw ()
{
	std::string hubId = _hubId.GetString ();
	Transport transport (_transport.GetString ());
	if (hubId.empty ())
		TheOutput.Display ("Please enter a valid hub name");
	else if (transport.IsUnknown ())
		TheOutput.Display ("Please enter a valid transport");
	else
	{
		_hubData.SetHubId (hubId);
		_hubData.SetTransport (transport);
		EndOk ();
	}
	return true;
}

bool GeneralPageHndlr::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_thisComputer.Init (dlgWin, IDC_JOIN_THIS_COMPUTER);
	_thisCluster.Init (dlgWin, IDC_JOIN_THIS_CLUSTER);
	_thisClusterHubId.Init (dlgWin, IDC_THIS_CLUSTER_HUB_ID);
	_remoteCluster.Init (dlgWin, IDC_JOIN_REMOTE_CLUSTER);
	_remoteClusterHubId.Init (dlgWin, IDC_JOIN_REMOTE_CLUSTER_HUB_ID);
	_advanced.Init (dlgWin, IDC_JOIN_REMOTE_CLUSTER_TRANSPORT);
	_projectName.Init (dlgWin, IDC_JOIN_LOCAL_PROJECTS);
	_sourcePath.Init (dlgWin, IDC_JOIN_PROJECT_SOURCE_EDIT);
	_browseSource.Init (dlgWin, IDC_JOIN_PROJECT_BROWSE_SOURCE);
	_userName.Init (dlgWin, IDC_JOIN_PROJECT_USER_NAME);
	_userPhone.Init (dlgWin, IDC_JOIN_PROJECT_USER_PHONE);

	std::string const & myHubId = _joinData.GetMyHubId (); 
	_thisClusterHubId.SetString (myHubId);
	InitRemoteClusterHubIds ();
	std::string const & adminHubId = _joinData.GetAdminHubId ();
	if (_iAmEmailPeer)
	{
		// Suggest another e-mail peer
		_remoteCluster.Check ();
		_thisClusterHubId.Disable ();
		_remoteClusterHubId.Enable ();
		_advanced.Enable ();
		_remoteClusterHubId.SetEditText (adminHubId.c_str ());
	}
	else
	{
		// I'm hub or satellite or standalone 
		if (adminHubId.empty () || _hubs.find (adminHubId) == _hubs.end () || adminHubId == myHubId)
		{
			// Suggest this cluster
			_thisCluster.Check ();
			_thisClusterHubId.Enable ();
			_remoteClusterHubId.Disable ();
			_advanced.Disable ();
		}
		else
		{
			// Suggest another cluster
			_remoteCluster.Check ();
			_thisClusterHubId.Disable ();
			_remoteClusterHubId.Enable ();
			_advanced.Enable ();
			_remoteClusterHubId.SetEditText (adminHubId.c_str ());
		}
	}

	for (NocaseSet::const_iterator it = _projects.begin (); it != _projects.end (); ++it)
	{
		_projectName.AddToList (it->c_str ());
	}

	// Limit the number of characters the user can type in the source path edit field
	_sourcePath.LimitText (FilePath::GetLenLimit ());
	_sourcePath.SetText (_joinData.GetProject ().GetRootDir ());
	CurrentMemberDescription description;
	_userName.SetString (description.GetName ());
	_userPhone.SetString (description.GetComment ());
	return true;
}

bool GeneralPageHndlr::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_JOIN_THIS_COMPUTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_thisClusterHubId.Disable ();
			_remoteClusterHubId.Disable ();
			_advanced.Disable ();
			return true;
		}
		break;
	case IDC_JOIN_THIS_CLUSTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_thisClusterHubId.Enable ();
			_remoteClusterHubId.Disable ();
			_advanced.Disable ();
			return true;
		}
		break;
	case IDC_JOIN_REMOTE_CLUSTER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_thisClusterHubId.Disable ();
			_remoteClusterHubId.Enable ();
			_advanced.Enable ();
			return true;
		}
		break;
	case IDC_JOIN_REMOTE_CLUSTER_TRANSPORT:
		ChangeRemoteClusterHubId ();
		return true;
	case IDC_JOIN_PROJECT_BROWSE_SOURCE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			std::string path;
			if (BrowseForProjectRoot (path, GetWindow (), false))	// Joining
				_sourcePath.SetString (path);

			return true;
		}
		break;
	}

	return false;
}

void GeneralPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_joinData.Clear ();
}

void GeneralPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	std::string projectName = _projectName.RetrieveTrimmedEditText ();
	if (_localRecipients.FindActiveProject (projectName))
	{
		// User selected to join the project with the same name as
		// already existing project on this machine.
		std::string info ("You are already enlisted in the project ");
		info += projectName;
		info += " on this machine";
		if (_localRecipients.EnlistmentAwaitsFullSync (projectName))
			info += " and awaiting a full sync from the administrator.";
		else
			info += '.';
		info += "\n\nAre you sure that you want to create another identical enlistment?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
													Out::PromptStyle (Out::YesNo,
																	  Out::No,
																	  Out::Question),
													GetWindow ());
		if (userChoice == Out::No)
		{
			result = 1;	// Don't close dialog
			return;
		}
		// Else the user wants to join the same project more then one time
	}

	Project::Data & project = _joinData.GetProject ();
	project.SetRootPath (_sourcePath.GetString ());
	project.SetProjectName (projectName);

	MemberDescription & joinee = _joinData.GetThisUser ();
	joinee.SetName (_userName.GetTrimmedString ());
	joinee.SetComment (_userPhone.GetString ());

	if (_thisCluster.IsChecked () || _thisComputer.IsChecked ())
	{
		_joinData.SetRemoteAdmin (false);
		_joinData.SetAdminHubId (_joinData.GetMyHubId ());
		_joinData.SetAdminTransport (_joinData.GetIntraClusterTransportToMe ());
	}
	else
	{
		Assert (_remoteCluster.IsChecked ());
		_joinData.SetRemoteAdmin (true);
		std::string hubId = _remoteClusterHubId.RetrieveTrimmedEditText ();
		_joinData.SetAdminHubId (hubId);
		
		NocaseMap<Transport>::const_iterator it = _hubs.find (hubId);
		if (it != _hubs.end ())
		{
			_joinData.SetAdminTransport (it->second);
		}
		else
		{
			Transport transport (hubId);
			// if unknown, will be detected during validation
			_joinData.SetAdminTransport (transport);
		}
	}

	if (!_joinData.IsValid ())
	{
		_joinData.DisplayErrors (GetWindow ());
		result = 1;	// Don't close dialog
	}
	_joinData.SetValid (true);
}

void GeneralPageHndlr::OnHelp () const throw (Win::Exception)
{
	AppHelp::Display (AppHelp::ProjectJoin, "Join Existing Project", GetWindow ());
}

void GeneralPageHndlr::InitRemoteClusterHubIds ()
{
	_remoteClusterHubId.Empty ();
	std::string myHubId (_joinData.GetMyHubId ());
	for (NocaseMap<Transport>::const_iterator it = _hubs.begin (); it != _hubs.end (); ++it)
	{
		// Skip my own hub id
		if (IsNocaseEqual (it->first, myHubId))
			continue;

		_remoteClusterHubId.AddToList (it->first.c_str ());
	}
}

void GeneralPageHndlr::ChangeRemoteClusterHubId ()
{
	HubRemoteConfig cfg;
	std::string hubId = _remoteClusterHubId.RetrieveEditText ();
	if (!hubId.empty ())
	{
		cfg.SetHubId (hubId);
		NocaseMap<Transport>::const_iterator it = _hubs.find (hubId);
		if (it != _hubs.end ())
		{
			cfg.SetTransport (it->second);
		}
		else
		{
			Transport transport (hubId);
			if (!transport.IsUnknown ())
				cfg.SetTransport (transport);
		}
	}

	RemoteHubAdvancedCtrl ctrl (cfg);
	if (ThePrompter.GetData (ctrl))
	{
		_hubs [cfg.GetHubId ()] = cfg.GetTransport ();
		InitRemoteClusterHubIds ();
		_remoteClusterHubId.SelectString (cfg.GetHubId ().c_str ());
	}
}

bool OptionsPageHndlr::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_autoSynch.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_SYNCH);
	_autoFullSynch.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_FULL_SYNCH);
	_keepCheckedOut.Init (dlgWin, IDC_PROJ_OPTIONS_KEEP_CHECKED_OUT);
	_joinAsObserver.Init (dlgWin, IDC_JOIN_PROJECT_OBSERVER);

	_autoSynch.UnCheck ();
	_autoFullSynch.UnCheck ();
	_keepCheckedOut.UnCheck ();
	if (_joinData.IsObserver ())
		_joinAsObserver.Check ();
	if (_joinData.IsForceObserver ())
		_joinAsObserver.Disable ();
	return true;
}

bool OptionsPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	return false;
}

void OptionsPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_joinData.Clear ();
}

void OptionsPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	Project::Options & options = _joinData.GetOptions ();
	if (_autoSynch.IsChecked ())
		options.SetAutoSynch (true);
	if (_autoFullSynch.IsChecked ())
		options.SetAutoFullSynch (true);

	if (_keepCheckedOut.IsChecked ())
		options.SetKeepCheckedOut (true);
	if (_joinAsObserver.IsChecked ())
		_joinData.SetObserver (true);
}

void OptionsPageHndlr::OnHelp () const throw (Win::Exception)
{
	AppHelp::Display (AppHelp::ProjectJoin, "Join Existing Project", GetWindow ());
}

JoinProjectHndlrSet::JoinProjectHndlrSet (JoinProjectData & joinData,
										  NocaseSet const & projects,
										  NocaseMap<Transport> & hubs,
										  LocalRecipientList const & localRecipients,
										  bool iAmEmailPeer)
	: PropPage::HandlerSet ("Join Existing Project"),
	  _joinData (joinData),
	  _generalPageHndlr (_joinData, projects, hubs, localRecipients, iAmEmailPeer),
	  _optionsPageHndlr (_joinData)
{
	AddHandler (_generalPageHndlr, "General");
	AddHandler (_optionsPageHndlr, "Options");
}
