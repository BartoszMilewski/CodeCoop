// ----------------------------------
// (c) Reliable Software, 2006 - 2007
// ----------------------------------

#include "precompiled.h"
#include "ProjectOptionsEx.h"
#include "ProjectDb.h"
#include "Catalog.h"
#include "Permissions.h"
#include "Encryption.h"
#include "Registry.h"
#include "ProjectBlueprint.h"
#include "OutputSink.h"

Project::OptionsEx::OptionsEx (Project::Db const & projectDb, 
							   Encryption::KeyMan const & keyMan, 
							   Catalog & catalog)
  : _catalog (catalog),
	_keyMan (keyMan),
    _caption ("Options for the project "),
	_distributorLicensee (catalog.GetDistributorLicensee ()),
	_seatsTotal (_catalog.GetDistributorLicenseCount ()),
	_newKey (keyMan.GetKey ())
{
	_caption += projectDb.ProjectName ();
	SetAutoSynch (projectDb.IsAutoSynch ());
	SetAutoJoin (projectDb.IsAutoJoin ());
	SetKeepCheckedOut (projectDb.IsKeepCheckedOut ());
	SetAutoFullSynch (projectDb.IsAutoFullSynch ());
	SetBccRecipients (projectDb.UseBccRecipients ());
	MemberState thisUserState = projectDb.GetMemberState (projectDb.GetMyId ());
	SetIsReceiver (thisUserState.IsReceiver ());
	SetIsAdmin (thisUserState.IsAdmin ());
	_mayBecomeDistributor = (projectDb.MemberCount () == 1 && IsProjectAdmin ()),
	SetDistribution (thisUserState.IsDistributor ());
	SetNoBranching (thisUserState.NoBranching ());
	SetCheckoutNotification (thisUserState.IsCheckoutNotification ());
	unsigned nextDistributorNumber = catalog.GetNextDistributorNumber ();
	if (nextDistributorNumber > _seatsTotal)
		_seatsAvailable = 0;
	else
		_seatsAvailable = _seatsTotal - nextDistributorNumber;

	_isAutoInvite = Registry::GetAutoInvitationOptions (_autoInvitePath);
}

std::string const & Project::OptionsEx::GetEncryptionOriginalKey () const 
{ 
	return _keyMan.GetKey (); 
}

std::string const & Project::OptionsEx::GetEncryptionCommonKey () const 
{ 
	return _keyMan.GetCommonKey (); 
}

bool Project::OptionsEx::ValidateAutoInvite (Win::Dow::Handle dlgWin) const
{
	if (!_isAutoInvite)
		return true;

	return ValidateAutoInvite (_autoInvitePath, _catalog, dlgWin);
}

bool Project::OptionsEx::ValidateAutoInvite (std::string const & autoInvitePath,
											 Catalog & catalog,
											 Win::Dow::Handle dlgWin)
{
	FilePath path (autoInvitePath);
	if (Project::Blueprint::IsRootPathWellFormed (path))
	{
		int projId;
		if (!catalog.IsPathInProject (path, projId))
			return true;

		std::string info ("The following folder:\n\n");
		info += autoInvitePath;
		info += "\n\nalready contains a Code Co-op project.";
		TheOutput.Display (info.c_str (), Out::Information, dlgWin);
	}
	else
	{
		Project::Blueprint::DisplayPathErrors (path, dlgWin);
	}

	return false;
}
