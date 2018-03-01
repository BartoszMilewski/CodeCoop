//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProgramOptions.h"
#include "Registry.h"
#include "Catalog.h"
#include "Validators.h"
#include "EmailConfig.h"

#include <Sys/Date.h>

ProgramOptions::Resend::Resend ()
	: _changesDetected (false),
	  _delay (10),
	  _repeat (1440)
{
	Registry::UserDispatcherPrefs dispatcherPrefs;
	if (!dispatcherPrefs.GetResendDelay (_delay))
	{
		// Resend was not setup yet, but now the user can change this
		_changesDetected = true;
	}
	if (!dispatcherPrefs.GetRepeatInterval (_repeat))
	{
		// Resend was not setup yet, but now the user can change this
		_changesDetected = true;
	}
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::Resend const & resendOptions)
{
	for (unsigned i = 0; i < 2; ++i)
	{
		os << (i == 0 ? "Re-send request delay: " : "; repeat interval: ");
		unsigned value = (i == 0 ? resendOptions.GetDelay () : resendOptions.GetRepeatInterval ());
		switch (value)
		{
		case 5:
			os << "5 minutes";
			break;
		case 10:
			os << "10 minutes";
			break;
		case 15:
			os << "15 minutes";
			break;
		case 20:
			os << "20 minutes";
			break;
		case 25:
			os << "25 minutes";
			break;
		case 30:
			os << "half an hour";
			break;
		case 60:
			os << "one hour";
			break;
		case 120:
			os << "two hours";
			break;
		case 180:
			os << "three hours";
			break;
		case 240:
			os << "four hours";
			break;
		case 300:
			os << "five hours";
			break;
		case 360:
			os << "six hours";
			break;
		case 720:
			os << "half a day";
			break;
		case 1440:
			os << "one day";
			break;
		default:
			os << "unknown: " << value;
			break;
		}
	}
	return os;
}

ProgramOptions::Update::Update ()
	: _isOn (true),
	  _isBackgroundDownload (false),
	  _period (UpdatePeriodValidator::GetDefault ()),
	  _changesDetected (false)
{
	Registry::UserDispatcherPrefs dispatcherPrefs;
	int year, month, day;
	if (dispatcherPrefs.GetUpdateTime (year, month, day))
	{
		// Update registry keys exist
		Date nextCheck (month, day, year);
		_isOn = nextCheck.IsValid ();
		_isBackgroundDownload = !dispatcherPrefs.IsConfirmUpdate ();
		unsigned int regPeriod = dispatcherPrefs.GetUpdatePeriod ();
		UpdatePeriodValidator checkPeriod (regPeriod);
		if (checkPeriod.IsValid ())
			_period = regPeriod;
	}
	else
	{
		// Co-op update was not setup yet, but now the user can change this
		_changesDetected = true;
	}
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::Update const & updateOptions)
{
	os << (updateOptions.IsAutoUpdate () ? "Automatic" : "Manual") << " update checking; ";
	os << (updateOptions.IsBackgroundDownload () ? "Background" : "Manual") << " download of new version; ";
	os << "check period: " << updateOptions.GetUpdateCheckPeriod () << " days";
	return os;
}

ProgramOptions::ChunkSize::ChunkSize (Catalog & catalog)
	: _canChange (true),
	  _changesDetected (false)
{
	Email::RegConfig email;
	if (email.IsValuePresent (Email::RegConfig::EMAIL_SIZE_NAME))
	{
		// Chunk size registry value exist
		_chunkSize = email.GetMaxEmailSize ();
	}
	else
	{
		// Co-op max chunk size was not setup yet, but now the user can change this
		_changesDetected = true;
	}
	Topology myTopology = catalog.GetMyTopology ();
	_canChange = myTopology.IsHubOrPeer () || myTopology.IsRemoteSatellite () || myTopology.IsTemporaryHub ();
}

ProgramOptions::Invitations::Invitations (Catalog & catalog)
	: _changesDetected (false),
	  _catalog (catalog)
{
	_isAutoInvitation = Registry::GetAutoInvitationOptions (_projectPath);
}

void ProgramOptions::Invitations::SetAutoInvite (bool flag)
{
	if (flag != _isAutoInvitation)
	{
		_changesDetected = true;
		_isAutoInvitation = flag;
	}
}

void ProgramOptions::Invitations::SetAutoInvitePath (std::string const & path)
{
	if (!::IsFileNameEqual (path, _projectPath))
	{
		_changesDetected = true;
		_projectPath = path;
	}
}

ProgramOptions::ScriptConflict::ScriptConflict ()
	: _changesDetected (false)
{
	_isResolveQuietly = Registry::IsQuietConflictOption ();
}

void ProgramOptions::ScriptConflict::SetResolveQuietly (bool flag)
{
	if (flag != _isResolveQuietly)
	{
		_changesDetected = true;
		_isResolveQuietly = flag;
	}
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::Invitations const & invitationOptions)
{
	os << "Auto-accept invitation: " << (invitationOptions.IsAutoInvitation () ? "yes" : "no") << std::endl;
	os << "New projects are created in the folder: " << invitationOptions.GetProjectFolder ();
	return os;
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::ChunkSize const & chunkingOptions)
{
	os << "Max chunk size: " << chunkingOptions.GetChunkSize () << "kB";
	return os;
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::ScriptConflict const & scriptConflictOptions)
{
	os << "When there is a conflict in which my scripts are not rejected (e.g., I'm a passive server) then "
		<< (scriptConflictOptions.IsResolveQuietly () ? "resolve it quietly." : "warn me.");
	return os;
}

std::ostream & operator<<(std::ostream & os, ProgramOptions::Data const & options)
{
	os << "*" << options.GetResendOptions () << std::endl;
	os << "*" << options.GetUpdateOptions () << std::endl;
	os << "*" << options.GetChunkSizeOptions () << std::endl;
	os << "*" << options.GetScriptConflictOptions () << std::endl;
	os << "*" << options.GetInitationOptions ();
	return os;
}
