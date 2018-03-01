// ----------------------------------
// (c) Reliable Software, 2006 - 2008
// ----------------------------------

#include "precompiled.h"
#include "ProjectInviteData.h"
#include "OutputSink.h"
#include "Registry.h"

#include <Mail/EmailAddress.h>
#include <StringOp.h>

static char const preferencesInvitationTypeValueName [] = "Recent invitation";
static char const preferencesFTPKeyName [] = "Invitation FTP site";
static char const preferencesLocalFolderValueName [] = "Invitation folder";

Project::InviteData::InviteData (std::string const & projectName)
	: _projectName (projectName)
{
	Registry::UserPreferences prefs;
	std::string invitationType = prefs.GetOption (preferencesInvitationTypeValueName);
	if (invitationType == "internet")
	{
		_copyRequest.SetInternet ();
	}
	else if (invitationType == "local")
	{
		_copyRequest.SetMyComputer ();
	}
	Ftp::SmartLogin & login = _copyRequest.GetFtpLogin ();
	login.RetrieveUserPrefs (preferencesFTPKeyName);
	SetLocalFolder (prefs.GetFilePath (preferencesLocalFolderValueName));
}

Project::InviteData::~InviteData ()
{
	if (!_options.test (ChangesDetected))
		return;

	try
	{
		Registry::UserPreferences prefs;
		if (IsManualInvitationDispatch () || IsTransferHistory ())
		{
			if (IsStoreOnInternet ())
			{
				prefs.SaveOption (preferencesInvitationTypeValueName, "internet");
				Ftp::SmartLogin & login = _copyRequest.GetFtpLogin ();
				login.RememberUserPrefs (preferencesFTPKeyName);
			}
			else
			{
				Assert (IsStoreOnLAN () || IsStoreOnMyComputer ());
				prefs.SaveOption (preferencesInvitationTypeValueName, "local");
				prefs.SaveFilePath (preferencesLocalFolderValueName, GetLocalFolder ());
			}
		}
		else
		{
			prefs.SaveOption (preferencesInvitationTypeValueName, "automatic");
		}
	}
	catch (...)
	{}
}

bool Project::InviteData::IsValid ()
{
	if (_userName.empty ())
		return false;

	if (!Email::IsValidAddress (_emailAddress))
		return false;
	
	if (IsOnSatellite ())
	{
		if (_computerName.empty ())
			return false;

		// set of valid characters includes:
		//	- letters, 
		//  - numbers, and 
		//  - the following symbols: ! @ # $ % ^ & ' ) ( . - _ { } ~ .
		// see SetComputerName API for reference
		std::string const validSymbols ("!@#$%^&')(.-_{}~");
		for (unsigned int i = 0; i < _computerName.size (); ++i)
		{
			char c = _computerName [i];
			if (!IsAlnum (c) && (validSymbols.find (c) == std::string::npos))
			{
				return false;
			}
		}
	}

	if (IsManualInvitationDispatch () || IsTransferHistory ())
		return _copyRequest.IsValid ();

	return true;
}

void Project::InviteData::DisplayErrors (Win::Dow::Handle owner) const
{
	if (_userName.empty ())
	{
		TheOutput.Display ("Please specify a name of the user you are inviting.",
							Out::Information, 
							owner);
	}
	else if (!Email::IsValidAddress (_emailAddress))
	{
		TheOutput.Display ("Please specify a valid e-mail address of the user you are inviting.\n"
							"If the user is on a satellite, specify their hub's email.",
						   Out::Information, 
						   owner);
	}
	else if (IsManualInvitationDispatch () || IsTransferHistory ())
	{
		if (!IsStoreOnInternet () && !IsStoreOnMyComputer () && !IsStoreOnLAN ())
		{
			std::string info ("Please, specify where to store project ");
			if (IsManualInvitationDispatch () && IsTransferHistory ())
				info += "invitation and history.";
			else if (IsManualInvitationDispatch ())
				info += "invitation.";
			else
				info += "history.";
			TheOutput.Display (info.c_str (), Out::Information, owner);
			return;
		}

		if (IsStoreOnInternet ())
		{
			_copyRequest.DisplayErrors (owner);
		}
		else
		{
			Assert (IsStoreOnMyComputer () || IsStoreOnLAN ());
			std::string const & localFolder = _copyRequest.GetLocalFolder ();
			if (localFolder.empty ())
			{
				std::string info ("Please, specify a folder path for the project ");
				if (IsManualInvitationDispatch () && IsTransferHistory ())
					info += "invitation and history.";
				else if (IsManualInvitationDispatch ())
					info += "invitation.";
				else
					info += "history.";
				TheOutput.Display (info.c_str (), Out::Information, owner);
			}
			else
			{
				_copyRequest.DisplayErrors (owner);
			}
		}
	}
	else if (IsOnSatellite ())
	{
		if (_computerName.empty ())
			TheOutput.Display ("When inviting a user on a satellite, you must specify their computer name.",
							   Out::Information, 
							   owner);
		else
			TheOutput.Display ("The satellite computer name you specified contains illegal characters.\n"
							   "Please specify a valid satellite computer name.",
								Out::Information, 
								owner);
	}
}

Project::OpenInvitationRequest::OpenInvitationRequest ()
	: OpenFileRequest ("Open Code Co-op Project Invitation",
					   "The project invitation is present on:  ",
					   "Select Project Invitation To Open",
					   "Recent opened invitation",
					   "Open invitation FTP site",
					   "Open invitation folder")
{}


