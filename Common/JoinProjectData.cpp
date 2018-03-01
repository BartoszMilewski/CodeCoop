// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "precompiled.h"
#include "JoinProjectData.h"
#include "Catalog.h"
#include "OutputSink.h"
#include "UserIdPack.h"
#include "Registry.h"

#include <Mail/EmailAddress.h>

JoinProjectData::JoinProjectData (Catalog & catalog, bool forceObserver, bool isInvitation)
	: Project::Blueprint (catalog),
	  _forceObserver (forceObserver),
	  _observer (forceObserver),
	  _remoteAdmin (false),
	  _configureFirst (false),
	  _isInvitation (isInvitation),
	  _isValid (false),
	  _myHubId (catalog.GetHubId ()),
	  _myTransport (catalog.GetInterClusterTransport (_myHubId))
{
	// Read project join preferences from the registry
	Registry::UserPreferences prefs;
	SetAdminHubId (prefs.GetOption ("Admin Hub Id"));
}

JoinProjectData::~JoinProjectData ()
{
	try
	{
		if (_isValid)
		{
			// Save project join preferences in the registry
			Registry::UserPreferences prefs;
			prefs.SaveOption ("Admin Hub Id", GetAdminHubId ());
		}
	}
	catch ( ... )
	{
		// Ignore all exceptions
	}
}

void JoinProjectData::Clear ()
{
	Project::Blueprint::Clear ();
	_forceObserver = false;
    _observer = false;
	_remoteAdmin = false;
	_configureFirst = false;
	_isInvitation = false;
	_isValid = false;
	_adminHubId.Assign ("");
	_adminTransport.Init ("");
	_scriptPath.clear ();
}

bool JoinProjectData::IsValid () const
{
	if (!Project::Blueprint::IsValid ())
		return false;

	if (!Email::IsValidAddress (_adminHubId))
		return false;

	if (IsRemoteAdmin ())
	{
		// Remote administrator must have transport and his hub id cannot be equal to this cluster hub id
		return !_adminTransport.IsUnknown () && !IsNocaseEqual (_adminHubId, _myHubId);
	}

	// Administrator is in this cluster -- hub id must be equal to this cluster hub id
	return IsNocaseEqual (_adminHubId, _myHubId);
}

void JoinProjectData::DisplayErrors (Win::Dow::Handle winOwner) const
{
	if (!Project::Blueprint::IsValid ())
	{
		Project::Blueprint::DisplayErrors (winOwner);
	}
	else if (!Email::IsValidAddress (_adminHubId))
	{
		TheOutput.Display ("Please specify a valid e-mail address of an existing hub\n"
						   "to which you want to send this join request.",
						   Out::Information, winOwner);
	}
	else if (IsRemoteAdmin ())
	{
		if (IsNocaseEqual (_adminHubId, _myHubId))
		{
			TheOutput.Display ("You cannot use your own hub email address\n"
							   "if the invitation came from a different hub or peer.",
							   Out::Information, winOwner);
		}
		else if (_adminTransport.IsUnknown ())
		{
			TheOutput.Display ("There is no transport associated with the target hub name.\n"
							   "Please use the Advanced... button to enter the transport.",
							   Out::Information, winOwner);
		}
	}
	else if (!IsNocaseEqual (_adminHubId, _myHubId))
	{
		std::string info ("When joining the project located in this cluster you have to use the following hub id:\n\n");
		info += _myHubId;
		TheOutput.Display (info.c_str (), Out::Information, winOwner);
	}
}

std::string JoinProjectData::GetNamedValues ()
{
	// Project join named values:
	//		Project::Blueprint names values followed by:
	//		recipient:"recipHubId"
	//      state:"observer"
	//      script:"full\synch\path"
	std::string namedValues (Project::Blueprint::GetNamedValues ());
	namedValues += " recipient:\"";
	namedValues += _adminHubId;
	namedValues += "\"";
	if (IsObserver ())
		namedValues += " state:\"observer\"";
	if (IsInvitation ())
		namedValues += " isInvitation:\"yes\"";
	if (!_scriptPath.empty ())
	{
		namedValues += " script:\"";
	    namedValues += _scriptPath;
		namedValues += "\"";
	}

	return namedValues;
}

void JoinProjectData::ReadNamedValues (NamedValues const & input)
{
	Project::Blueprint::ReadNamedValues (input);
	_adminHubId = input.GetValue ("recipient");
	_observer = (input.GetValue ("state") == "observer");
	_isInvitation = (input.GetValue ("isInvitation") == "yes");
	_scriptPath = input.GetValue ("script");
	_remoteAdmin = !IsNocaseEqual (_adminHubId, _myHubId);
	_adminTransport = Transport (_adminHubId);
}
