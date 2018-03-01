// ----------------------------------
// (c) Reliable Software, 2000 - 2006
// ----------------------------------
#include "precompiled.h"
#include "UnknownRecipWiz.h"
#include "Config.h"
#include "BrowseForFolder.h"
#include "UserIdPack.h"
#include "OutputSink.h"
#include "DispatcherParams.h"
#include "resource.h"

#include <Ctrl/Output.h>

using namespace UnknownRecipient;

//-----------
// Standalone
//-----------

bool StandaloneCtrl::GoNext (long & result)
{
	Assert (_dlgData._answer == Data::Cluster);
	result = IDD_UNKNOWN_HUB_OR_SAT;
	return true;
}

bool StandaloneCtrl::OnInitDialog () throw (Win::Exception)
{
	_project.Init (GetWindow (), IDC_PROJECT);
	_hubId.Init (GetWindow (), IDC_EMAIL);
	_hubWithEmail.Init (GetWindow (), IDC_HUB);
	_lan.Init (GetWindow (), IDC_LAN);

	if (_dlgData._isJoinRequest)
	{
		_mistake.Init (GetWindow (), IDC_MISTAKE);
	}
	else
	{
		_userId.Init (GetWindow (), IDC_LOCATION);
		_comment.Init (GetWindow (), IDC_COMMENT);
	}
    _project.SetString (_dlgData.GetAddress ().GetProjectName ());
    _hubId.SetString (_dlgData.GetAddress ().GetHubId ());
	
	if (!_dlgData._isJoinRequest)
	{
		UserIdPack pack (_dlgData.GetAddress ().GetUserId ());
		_userId.SetString (pack.GetUserIdString ());
		_comment.SetString (_dlgData._comment);
	}

	_hubWithEmail.Check ();
	SetButtons (PropPage::Wiz::Finish);
	// set default result
	_dlgData._answer = Data::Remote;
	return true;
}

bool StandaloneCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_LAN:
			_dlgData._answer = Data::Cluster;
			SetButtons (PropPage::Wiz::Next);
			break;
	case IDC_HUB:
			_dlgData._answer = Data::Remote;
			SetButtons (PropPage::Wiz::Finish);
			break;
	case IDC_MISTAKE:
			_dlgData._answer = Data::Ignore;
			SetButtons (PropPage::Wiz::Finish);
			break;
	default:
		return false;
	}
	return true;
}

//------------------
// Standalone to LAN
//------------------

bool Standalone2LanCtrl::GoNext (long & result)
{
	Assert (_dlgData._answer == Data::Cluster);
	if (_dlgData._wantToBeSat)
		result = IDD_UNKNOWN_HUB_PATH;
	else
		result = IDD_UNKNOWN_SAT_PATH;
	return true;
}

void Standalone2LanCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::Next);
}

bool Standalone2LanCtrl::OnInitDialog () throw (Win::Exception)
{
	_project.Init (GetWindow (), IDC_PROJECT);
	_hubId.Init (GetWindow (), IDC_EMAIL);
	_hub.Init (GetWindow (), IDC_HUB);
	_sat.Init (GetWindow (), IDC_SAT);

	if (_dlgData._isJoinRequest)
	{
		_mistake.Init (GetWindow (), IDC_MISTAKE);
	}
	else
	{
		_userId.Init (GetWindow (), IDC_LOCATION);
		_comment.Init (GetWindow (), IDC_COMMENT);
	}	
    _project.SetString (_dlgData.GetAddress ().GetProjectName ());
    _hubId.SetString (_dlgData.GetAddress ().GetHubId ());
	
	if (!_dlgData._isJoinRequest)
	{
		UserIdPack pack (_dlgData.GetAddress ().GetUserId ());
		_userId.SetString (pack.GetUserIdString ());
		_comment.SetString (_dlgData._comment);
	}

	// most probably this machine is to be satellite
	_sat.Check ();

	// set default result
	_dlgData._answer = Data::Cluster;
	_dlgData._wantToBeSat = true;
	return true;
}

bool Standalone2LanCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_HUB:
			_dlgData._answer = Data::Cluster;
			_dlgData._wantToBeSat = false;
			SetButtons (PropPage::Wiz::Next);
			break;
	case IDC_SAT:
			_dlgData._answer = Data::Cluster;
			_dlgData._wantToBeSat = true;
			SetButtons (PropPage::Wiz::Next);
			break;
	case IDC_MISTAKE:
			_dlgData._answer = Data::Ignore;
			SetButtons (PropPage::Wiz::Finish);
			break;
	default:
		return false;
	}
	return true;
}

//-------------
// Project Name
//-------------

void ProjectNameCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::Next);
}

bool ProjectNameCtrl::GoNext (long & result)
{
	if (_misspelled.IsChecked ())
	{
		_dlgData._answer = Data::Ignore;
		result = IDD_UNKNOWN_MISSPELLED;
	}
	else // correct checked
	{
		if (_dlgData._isJoinRequest)
			result = IDD_UNKNOWN_INTRO_JOIN;
		else
			result = IDD_UNKNOWN_INTRO;
	}
	return true;
}

bool ProjectNameCtrl::OnInitDialog () throw (Win::Exception)
{
	_name.Init (GetWindow (), IDC_NAME);
	_projectList.Init (GetWindow (), IDC_LIST);
	_comment.Init (GetWindow (), IDC_COMMENT);
	_misspelled.Init (GetWindow (), IDC_MISSPELLED);
	_correct.Init (GetWindow (), IDC_CORRECT);

    _name.SetString (_dlgData.GetAddress ().GetProjectName ());
	// initialize with project list
	typedef NocaseSet::const_iterator iterator;
	for (iterator it = _dlgData.GetProjectList ().begin ();
		it != _dlgData.GetProjectList ().end (); ++it)
	{
		_projectList.AddItem (it->c_str ());
	}

	_comment.SetString (_dlgData._comment);

	_misspelled.Check ();
	return true;
}

//-----------
// Misspelled
//-----------

void MisspelledCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
}

//---------------
// Unknown Hub ID
//---------------

void UnknownHubIdCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::Next);
}

bool UnknownHubIdCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_IS_REMOTE:
			_dlgData._answer = Data::Remote;
			SetButtons (PropPage::Wiz::Next);
			break;
	case IDC_MISTAKE:
			_dlgData._answer = Data::Ignore;
			SetButtons (PropPage::Wiz::Finish);
			break;
	default:
		return false;
	}
	return true;
}

bool UnknownHubIdCtrl::GoNext (long & result)
{
	Assert (_remote.IsChecked ());
	result = IDD_UNKNOWN_USE_EMAIL;
	return true;
}

bool UnknownHubIdCtrl::OnInitDialog () throw (Win::Exception)
{
	_descriptionText.Init (GetWindow (), IDC_DESCRIPTION);
	_name.Init (GetWindow (), IDC_NAME);
	_hubId.Init (GetWindow (), IDC_EMAIL);
	_comment.Init (GetWindow (), IDC_COMMENT);
	_remote.Init (GetWindow (), IDC_IS_REMOTE);
	_mistake.Init (GetWindow (), IDC_MISTAKE);

	if (_dlgData._isJoinRequest)
	    if (_dlgData._isSenderLocal)
			_descriptionText.SetText ("You are sending a Join Request script from this machine");
		else
			_descriptionText.SetText ("A satellite user is sending a Join Request script through this machine");
	else
	    if (_dlgData._isSenderLocal)
			_descriptionText.SetText ("You are sending a script from this machine");
		else
			_descriptionText.SetText ("A satellite user is sending a script through this machine");

	_name.SetString (_dlgData.GetAddress ().GetProjectName ());
    _hubId.SetString (_dlgData.GetAddress ().GetHubId ());
	_comment.SetString (_dlgData._comment);

	_remote.Check ();
	return true;
}

// ---------
// Addressee
//----------

void AddresseeCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
}

//------
// Intro
//------

void IntroCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	if (_dlgData._isJoinRequest)
	{
		if (_dlgData._isIncoming)
		{
			SetButtons (PropPage::Wiz::Next);
		}
		else
		{
			if (_dlgData._isSenderLocal)
				SetButtons (PropPage::Wiz::BackFinish);
			else
				SetButtons (PropPage::Wiz::NextBack);
		}
	}
	else
	{
		// not join, incoming or outgoing
		if (_ignore.IsChecked ())
			SetButtons (PropPage::Wiz::Finish);
		else
			SetButtons (PropPage::Wiz::Next);
	}
}

bool IntroCtrl::GoNext (long & result)
{
	if (_ignore.IsChecked ())
	{
		// must be join. there is no Next button in other cases.
		Assert (_dlgData._isJoinRequest);
		_dlgData._answer = Data::Ignore;
		result = IDD_UNKNOWN_ADDRESSEE;
	}
	else if (_isCluster.IsChecked ())
	{
		_dlgData._answer = Data::Cluster;
		result = IDD_UNKNOWN_SAT_PATH;
	}
	else // isRemote checked
	{
		if (_dlgData._isHubIdKnown)
			result = IDD_UNKNOWN_CONFIG_ERROR;
		else
			result = IDD_UNKNOWN_USE_EMAIL;
	}
	return true;
}

void IntroCtrl::OnFinish (long & result) throw (Win::Exception)
{
	if (_ignore.IsChecked ())
	{
		_dlgData._answer = Data::Ignore;
	}
	else
	{
		_dlgData._answer = Data::Remote;
	}
}

bool IntroCtrl::OnInitDialog () throw (Win::Exception)
{
	_name.Init (GetWindow (), IDC_NAME);
	_hubId.Init (GetWindow (), IDC_EMAIL);
	if (!_dlgData._isJoinRequest)
		_userId.Init (GetWindow (), IDC_LOCATION);

	_comment.Init (GetWindow (), IDC_COMMENT);
	_ignore.Init (GetWindow (), IDC_NOT_IN_PROJECT);
	_isCluster.Init (GetWindow (), IDC_ON_SATELLITE);
	if (!_dlgData._isIncoming)
		_isRemote.Init (GetWindow (), IDC_IS_REMOTE);

    _name.SetString (_dlgData.GetAddress ().GetProjectName ());
	if (!_dlgData._isJoinRequest)
	{
		UserIdPack pack (_dlgData.GetAddress ().GetUserId ());
		_userId.SetString (pack.GetUserIdString ());
	}
    _hubId.SetString (_dlgData.GetAddress ().GetHubId ());
	_comment.SetString (_dlgData._comment);

	_ignore.Check ();
	return true;
}

bool IntroCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_NOT_IN_PROJECT:
		if (_dlgData._isIncoming)
		{
			if (_dlgData._isJoinRequest)
				SetButtons (PropPage::Wiz::Next);
			else
				SetButtons (PropPage::Wiz::Finish);
		}
		else
		{
			if (_dlgData._isJoinRequest)
			{
				if (_dlgData._isSenderLocal)
					SetButtons (PropPage::Wiz::BackFinish);
				else
					SetButtons (PropPage::Wiz::NextBack);
			}
			else
				SetButtons (PropPage::Wiz::Finish);
		}
		break;
	case IDC_ON_SATELLITE:
	case IDC_IS_REMOTE:
		if (_dlgData._isIncoming)
		{
			SetButtons (PropPage::Wiz::Next);
		}
		else
		{
			if (_dlgData._isJoinRequest)
				SetButtons (PropPage::Wiz::NextBack);
			else
				SetButtons (PropPage::Wiz::Next);
		}
		break;
	default:
		return false;
	}
	return true;
}

//-----------
// Hub or Sat
//-----------

void HubOrSatCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::NextBack);
}

bool HubOrSatCtrl::GoNext (long & result)
{
	Transport email (_hubId.GetString ());
	if (email.IsUnknown ())
	{
		TheOutput.Display ("Please enter a valid hub email address (or other transport)");
		result = -1;
		return false;
	}
	_dlgData.SetInterClusterTransportToMe (email);

	_dlgData._wantToBeSat = _isSat.IsChecked ();
	if (_dlgData._wantToBeSat)
		result = IDD_UNKNOWN_HUB_PATH;
	else
		result = IDD_UNKNOWN_SAT_PATH;
	return true;
}

bool HubOrSatCtrl::OnInitDialog () throw (Win::Exception)
{
	_isHub.Init (GetWindow (), IDC_HUB);
	_isSat.Init (GetWindow (), IDC_SAT);
	_hubId.Init (GetWindow (), IDC_HUB_ID);

	_isHub.Check ();
	return true;
}

//-----
// Path
//-----

void PathCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
}

void PathCtrl::OnFinish (long & result) throw (Win::Exception)
{
	result = 0;
	Transport transport (_pathCombo.RetrieveEditText ());
	_dlgData.InitTransport (transport);
	if (!TheTransportValidator->ValidateExcludePublicInbox (
			transport, 
			"You cannot specify your own Public Inbox\n"
			"as the forwarding path to your satellite.\n\n"
			"If you don't have satellites, take a step back and select:\n"
			"\"Ignore this user.\"\n"
			"This user has probably already defected.",
			GetWindow ()))
	{
		result = -1;
	}
}

bool PathCtrl::OnInitDialog () throw (Win::Exception)
{
	_pathCombo.Init (GetWindow (), IDC_COMBO);
	_browse.Init (GetWindow (), IDB_BROWSE);
	std::set<Transport> const & transports = _dlgData.GetTransports ();
	typedef std::set<Transport>::const_iterator iterator;
	for (iterator it = transports.begin (); it != transports.end (); ++it)
	{
		_pathCombo.AddToList (it->GetRoute ().c_str ());
	}
	_pathCombo.SetSelection (0);
	return true;
}

bool PathCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDB_BROWSE:
		if (_browse.IsClicked (notifyCode))
		{
			std::string folder;
			if (BrowseForSatelliteShare (folder, GetWindow ()))
			{
				char const * path = folder.c_str ();
				int idx = _pathCombo.FindString (path);
				if (idx != -1)
					_pathCombo.SetSelection (idx);
				else
				{
					_pathCombo.AddToList (path);
					_pathCombo.SelectString (path);
				}
			}
			return true;
		}
		break;
	};
	return false;
}

//----------
// ConfigErr
//----------

void ConfigErrCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
}

void ConfigErrCtrl::OnFinish (long & result) throw (Win::Exception)
{
	_dlgData._answer = Data::Remote;
}

bool ConfigErrCtrl::OnInitDialog () throw (Win::Exception)
{
	_hubIdEdit.Init (GetWindow (), IDC_EMAIL);
    _hubIdEdit.SetString (_dlgData.GetAddress ().GetHubId ());
	return true;
}

// ---------
// Use Email
//----------

void UseEmailCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	SetButtons (PropPage::Wiz::BackFinish);
}

void UseEmailCtrl::OnFinish (long & result) throw (Win::Exception)
{
	_dlgData._answer = Data::UseEmail;
}

// -----------
// The Wizards
// -----------

UnknownRecipient::Standalone2LanHandlerSet::Standalone2LanHandlerSet (
			UnknownRecipient::Data & unknownRecipData, 
			bool isJoinRequest)
	: PropPage::HandlerSet (DispatcherTitle),
	  _standalone2LanCtrl (unknownRecipData, 
						   isJoinRequest? IDD_UNKNOWN_STANDALONE_TO_LAN_JOIN: 
										  IDD_UNKNOWN_STANDALONE_TO_LAN),
	  _pathCtrlHub (unknownRecipData, IDD_UNKNOWN_HUB_PATH),
	  _pathCtrlSat (unknownRecipData, IDD_UNKNOWN_SAT_PATH)
{
	AddHandler (_standalone2LanCtrl);
	AddHandler (_pathCtrlHub);
	AddHandler (_pathCtrlSat);
}

UnknownRecipient::Standalone2LanOrEmailHandlerSet::Standalone2LanOrEmailHandlerSet (
			UnknownRecipient::Data & unknownRecipData, 
			bool isJoinRequest)
	: PropPage::HandlerSet (DispatcherTitle),
	  _standaloneCtrl (unknownRecipData, 
				  	   isJoinRequest? IDD_UNKNOWN_STANDALONE_JOIN: 
										 IDD_UNKNOWN_STANDALONE),
	  _hubOrSatCtrl (unknownRecipData, IDD_UNKNOWN_HUB_OR_SAT),
	  _pathCtrlHub (unknownRecipData, IDD_UNKNOWN_HUB_PATH),
	  _pathCtrlSat (unknownRecipData, IDD_UNKNOWN_SAT_PATH)
{
	AddHandler (_standaloneCtrl);
	AddHandler (_hubOrSatCtrl);
	AddHandler (_pathCtrlHub);
	AddHandler (_pathCtrlSat);
}

UnknownRecipient::DefectedMisspelledOrClusterHandlerSet::DefectedMisspelledOrClusterHandlerSet (
			UnknownRecipient::Data & unknownRecipData)
	: PropPage::HandlerSet (DispatcherTitle),
	  _projectNameCtrl (unknownRecipData, IDD_UNKNOWN_PROJECT_NAME),
	  _misspelledCtrl (unknownRecipData, IDD_UNKNOWN_MISSPELLED),
	  _addresseeCtrl (unknownRecipData, IDD_UNKNOWN_ADDRESSEE),
	  _introCtrlLan (unknownRecipData, 
					 unknownRecipData.IsJoinRequest () ?
						IDD_UNKNOWN_LAN_OR_IGNORE_JOIN : IDD_UNKNOWN_LAN_OR_IGNORE),
	  _introCtrl (unknownRecipData, 
				  unknownRecipData.IsJoinRequest () ?
						IDD_UNKNOWN_INTRO_JOIN : IDD_UNKNOWN_INTRO),
	  _pathCtrl (unknownRecipData, IDD_UNKNOWN_SAT_PATH),
	  _errCtrl (unknownRecipData, IDD_UNKNOWN_CONFIG_ERROR)
{
	if (unknownRecipData.IsIncoming ())
		AddHandler (_introCtrlLan);
	else if (unknownRecipData.IsJoinRequest ())
		AddHandler (_projectNameCtrl);

	AddHandler (_introCtrl);
	AddHandler (_misspelledCtrl);
	AddHandler (_addresseeCtrl);
	AddHandler (_pathCtrl);
	AddHandler (_errCtrl);
}

UnknownRecipient::SwitchToEmailHandlerSet::SwitchToEmailHandlerSet (UnknownRecipient::Data & unknownRecipData)
: PropPage::HandlerSet (DispatcherTitle),
  _unknownHubIdCtrl (unknownRecipData, IDD_UNKNOWN_EMAIL),
  _useEmailCtrl (unknownRecipData, IDD_UNKNOWN_USE_EMAIL)
{
	AddHandler (_unknownHubIdCtrl);
	AddHandler (_useEmailCtrl);
}
