//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "precompiled.h"
#include "Registry.h"
#include "RegKeys.h"
#include "Validators.h"
#include "ScriptProcessorConfig.h"

#include <File/Path.h>
#include <Ex/WinEx.h>

bool Registry::UserDispatcherPrefs::IsFirstEmail () const
{
	unsigned long val = 0xffffffff;
	_prefs.Key ().GetValueLong ("First Email", val);
	return val == 1;
}

void Registry::UserDispatcherPrefs::SetFirstEmail (bool value)
{
	_prefs.Key ().SetValueLong ("First Email", value ? 1 : 0);
}

bool Registry::UserDispatcherPrefs::IsSimpleMapiForcedObsolete () const
{
	unsigned long val = 0xffffffff;
	if (_prefs.Key ().GetValueLong ("Mapi", val))
	{
		// val = 0 -- force simple Mapi; 1 -- full Mapi can be used
		return val == 0;
	}
	// Value not present -- nothing forced
	return false;
}

bool Registry::UserDispatcherPrefs::IsUsePop3Obsolete ()
{
	unsigned long val = 0xffffffff;
	if (_prefs.Key ().GetValueLong ("Use Pop3", val))
	{
		return val == 1;
	}
	else // value not present
		return false;
}

bool Registry::UserDispatcherPrefs::IsUseSmtpObsolete ()
{
	unsigned long val = 0xffffffff;
	if (_prefs.Key ().GetValueLong ("Use Smtp", val))
	{
		return val == 1;
	}
	else // value not present
		return false;
}

bool Registry::UserDispatcherPrefs::IsEmailLogging ()
{
	unsigned long val = 0;
	if (_prefs.Key ().GetValueLong ("Email Logging", val))
	{
		return val == 1;
	}
	else // value not present
		return false;
}

bool Registry::UserDispatcherPrefs::GetResendDelay (unsigned long & delay)
{
	if (_prefs.Key ().GetValueLong ("Auto Resend Delay", delay))
	{
		// Registry key present
		if (5 <= delay && delay <= 1440)
			return true;	// Registry value in valid range

		// Value out of valid range -- return default delay of 30 minutes
		delay = 30;
	}
	else
	{
		// No registry key -- return default delay of 30 minutes
		delay = 30;
	}
	return false;	// Registry key not presetn or has invalid value
}

bool Registry::UserDispatcherPrefs::GetRepeatInterval (unsigned long & repeat)
{
	if (_prefs.Key ().GetValueLong ("Auto Resend Repeat Interval", repeat))
	{
		// Registry key present
		if (30 <= repeat && repeat <= 1440)
			return true;	// Registry value in valid range

		// Value out of valid range -- return default repeat interval of one day (1440 minutes)
		repeat = 1440;
	}
	else
	{
		// No registry key -- return default repeat interval of one day (1440 minutes)
		repeat = 1440;
	}
	return false;	// Registry key not present or has invalid value
}

void Registry::UserDispatcherPrefs::SetAutoResend (unsigned delay, unsigned repeat)
{
	_prefs.Key ().SetValueLong ("Auto Resend Delay", delay);
	_prefs.Key ().SetValueLong ("Auto Resend Repeat Interval", repeat);
}

bool Registry::UserDispatcherPrefs::GetAutoReceivePeriodObsolete (unsigned long & period)
{
	if (_prefs.Key ().GetValueLong ("Auto Receive Period", period))
		return true;
	else
		return false;
}

bool Registry::UserDispatcherPrefs::GetMaxEmailSizeObsolete (unsigned long & size)
{
	if (_prefs.Key ().GetValueLong ("Max Email Size", size))
	{
		// Registry key present
		ChunkSizeValidator validator (size);
		if (validator.IsInValidRange ())
			return true;	// Registry value in valid range

		// Value out of valid range -- return default email size
		size = ChunkSizeValidator::GetDefaultChunkSize ();
	}
	else
	{
		// No registry key -- return default email size
		size = ChunkSizeValidator::GetDefaultChunkSize ();
	}
	return false;
}

void Registry::UserDispatcherPrefs::SetStayOffSiteHub (bool asked)
{
	_prefs.Key ().SetValueLong ("Stay Proxy", asked ? 1 : 0);
}

bool Registry::UserDispatcherPrefs::IsStayOffSiteHub ()
{
	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Stay Proxy", val);
	return val != 0;
}

long Registry::UserDispatcherPrefs::GetEmailStatusObsolete ()
{
	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Email Config Passed", val);
	return val;
}

void Registry::UserDispatcherPrefs::GetMainWinPlacement (Win::Placement & placement)
{
	RegKey::New placementKey (_prefs.Key (), "Main Window");
	RegKey::ReadWinPlacement (placement, placementKey);
}

void Registry::UserDispatcherPrefs::SaveMainWinPlacement (Win::Placement & placement)
{
	RegKey::New placementKey (_prefs.Key (), "Main Window");
	RegKey::SaveWinPlacement (placement, placementKey);
}

// post condition: unchanged size of widths
void Registry::UserDispatcherPrefs::GetColumnWidths (char const * viewName,
													 std::vector<int> & widths)
{
	std::fill (widths.begin (), widths.end (), 0);
	// Get record set registry key
	RegKey::New view (_prefs.Key (), viewName);
	RegKey::New columns (view, "Columns");
	// Get column widths from registry
	unsigned int const colCount = widths.size ();
	for (RegKey::ValueSeq colSeq (columns); !colSeq.AtEnd (); colSeq.Advance ())
	{
		unsigned int col = ToInt (colSeq.GetName ());
		if (col >= colCount)
			continue;
		
		widths [col] = colSeq.GetLong ();
	}
}

void Registry::UserDispatcherPrefs::SetColumnWidths (char const * viewName,
													 std::vector<int> const & widths)
{
	try 
	{
		// Get record set registry key
		RegKey::New view (_prefs.Key (), viewName);
		// Save column widths in registry
		RegKey::New columns (view, "Columns");
		std::string colName;
		for (unsigned int i = 0; i < widths.size (); i++)
		{
			colName = ToString (i);
			columns.SetValueLong (colName.c_str (), widths [i]);
		}
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

unsigned int Registry::UserDispatcherPrefs::GetSortCol (char const * viewName)
{
	RegKey::New view (_prefs.Key (), viewName);
	unsigned long col = 0;
	view.GetValueLong ("Sort Column", col);
	return col;
}

void Registry::UserDispatcherPrefs::SetSortCol (char const * viewName, unsigned int col)
{
	RegKey::New view (_prefs.Key (), viewName);
	view.SetValueLong ("Sort Column", col);
}

// Version updates
bool Registry::UserDispatcherPrefs::IsConfirmUpdate ()
{
	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Update Options", val);
	return val == 1;
}

bool Registry::UserDispatcherPrefs::GetUpdateTime (int & year, int & month, int & day)
{
	unsigned long l;
	if (!_prefs.Key ().GetValueLong ("Update Year",  l))
		return false;
	year = l;
	if (!_prefs.Key ().GetValueLong ("Update Month", l))
		return false;
	month = l;
	if (!_prefs.Key ().GetValueLong ("Update Day",   l))
		return false;
	day = l;
	return true;
}

unsigned int Registry::UserDispatcherPrefs::GetUpdatePeriod ()
{
	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Update Period", val);
	return val;
}

unsigned int Registry::UserDispatcherPrefs::GetLastBulletin ()
{
	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Last Bulletin", val);
	return val;
}

std::string Registry::UserDispatcherPrefs::GetLastDownloadedVersion ()
{
	return _prefs.Key ().GetStringVal ("Last Download");
}

void Registry::UserDispatcherPrefs::SetUpdateTime (int year, int month, int day)
{
	_prefs.Key ().SetValueLong ("Update Year",  year);
	_prefs.Key ().SetValueLong ("Update Month", month);
	_prefs.Key ().SetValueLong ("Update Day",   day);
}

void Registry::UserDispatcherPrefs::SetUpdatePeriod (unsigned int checkPeriod)
{
	_prefs.Key ().SetValueLong ("Update Period", checkPeriod);
}

void Registry::UserDispatcherPrefs::SetIsConfirmUpdate (bool isConfirmUpdate)
{
	_prefs.Key ().SetValueLong ("Update Options", isConfirmUpdate ? 1 : 0);
}

void Registry::UserDispatcherPrefs::SetLastBulletin (unsigned int lastBulletin)
{
	_prefs.Key ().SetValueLong ("Last Bulletin", lastBulletin);
}

void Registry::UserDispatcherPrefs::SetLastDownloadedVersion (std::string const & lastVersion)
{
	_prefs.Key ().SetValueString ("Last Download", lastVersion);
}

void Registry::UserDispatcherPrefs::ReadScriptProcessorConfigObsolete (ScriptProcessorConfig & cfg) 
{
	cfg.SetPreproCommand (_prefs.Key ().GetStringVal ("Prepro Command"));
	cfg.SetPreproResult (_prefs.Key ().GetStringVal ("Prepro Result"));
	cfg.SetPostproCommand (_prefs.Key ().GetStringVal ("Postpro Command"));
	cfg.SetPostproExt (_prefs.Key ().GetStringVal ("Postpro Extension"));

	unsigned long val = 0;
	_prefs.Key ().GetValueLong ("Prepro Needs Project Name", val);
	cfg.SetPreproNeedsProjName (val == 1);
	val = 0;
	_prefs.Key ().GetValueLong ("Prepro Can Send Unprocessed", val);
	cfg.SetCanSendUnprocessed (val == 1);
}
