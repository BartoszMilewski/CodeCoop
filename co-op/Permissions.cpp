//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"
#include "Permissions.h"
#include "Catalog.h"
#include "ProjectDb.h"
#include "Registry.h"
#include "PathFind.h"

Permissions::Permissions (Catalog & catalog)
	: _global (catalog),
	  _trial (catalog),
	  _projectDataValid (false)
{}

class ProjectLock
{
public:
	ProjectLock (PathFinder & finder)
		:_finder (finder)
	{
		_result = _finder.LockProject ();
	}
	~ProjectLock () throw ()
	{
		_finder.UnlockProject ();
	}
	bool WasFound () const { return _result; }

private:
	PathFinder &_finder;
	bool		_result;
};

void Permissions::TestProjectLockConsistency (PathFinder & pathFinder)
{
	ProjectLock lock (pathFinder);
	if (!lock.WasFound ())
	{
		// Lock is missing -- create New one
		pathFinder.CreateLock ();
		pathFinder.StampLockFile (_trial.GetTrialStart ());
	}
}

void Permissions::SetLicense (Catalog & catalog, License const & newLicense)
{
	catalog.SetLicense (newLicense);
	_global.Init (newLicense.GetLicenseString ());
}

void Permissions::RefreshProjectData (Project::Db const & projectDb, PathFinder & pathFinder)
{
	if (projectDb.GetMyId () != gidInvalid)
	{
		// Not awaiting the full sync script
		std::unique_ptr<MemberDescription> thisUser 
			= projectDb.RetrieveMemberDescription (projectDb.GetMyId ());
		_local.Init (thisUser->GetLicense ());
		_seats.Refresh (projectDb);
		_state = projectDb.GetMemberState (projectDb.GetMyId ());
		_projectDataValid = true;
	}
	_trial.Refresh (pathFinder);
}

void Permissions::XRefreshProjectData (Project::Db const & projectDb, PathFinder * pathFinder)
{
	if (projectDb.XGetMyId () != gidInvalid)
	{
		// Not awaiting the full sync script
		std::unique_ptr<MemberDescription> thisUser 
			= projectDb.XRetrieveMemberDescription (projectDb.XGetMyId ());
		_local.Init (thisUser->GetLicense ());
		_seats.XRefresh (projectDb);
		_state = projectDb.XGetMemberState (projectDb.XGetMyId ());
		_projectDataValid = true;
	}
	if (pathFinder != 0)
		_trial.Refresh (*pathFinder);
}

void Permissions::PropagateLocalLicense (Catalog & catalog)
{
	catalog.SetLicense (_local);
	_global.Init (_local.GetLicenseString ());
}

// Returns true if the global license has different licensee name
bool Permissions::IsChangedLicensee (License const & newLicense) const
{
	return _global.GetLicensee () != newLicense.GetLicensee ();
}

bool Permissions::IsChangedProduct (License const & newLicense) const
{
	return _global.GetProductId () != newLicense.GetProductId ();
}

void Permissions::ClearProjectData ()
{
	_local.Init ("");
	_seats.Clear ();
	_state.Reset ();
	_projectDataValid = false;
}

bool Permissions::HasValidLicense () const
{
	if (IsProjectDataValid ())
		return (_local.IsCurrentVersion () && _local.IsValidProduct ()) ||
			   (_global.IsCurrentVersion () && _global.IsValidProduct ());
	else
		return _global.IsCurrentVersion () && _global.IsValidProduct ();
}

bool Permissions::IsGlobalLicenseeEmpty () const
{
	return _global.GetLicensee ().empty ();
}

bool Permissions::CanCreateNewProject () const
{
	return _global.IsCurrentVersion () || IsTrial ();
}

bool Permissions::CanCreateNewBranch () const
{
	if (IsProjectDataValid ())
	{
		if (_state.IsReceiver ())
		{
			if (_state.NoBranching ())
				return _global.IsCurrentVersion ();
			else
				return HasValidLicense ();
		}
	}

	return HasValidLicense () || IsTrial ();
}

bool Permissions::CanExecuteSetScripts () const
{
	if (IsProjectDataValid ())
	{
		if (_state.IsReceiver ())
		{
			return _state.IsVoting ();
		}
		else
		{
			// Revisit: maybe we shouldn't allow unlicensed user after the trial to execute scripts?
			//return HasValidLicense () || IsTrial ();
			return true;
		}
	}

	return false;
}

bool Permissions::MayBecomeDistributor (Project::Db const & projDb) const
{
	Assert (IsProjectDataValid ());
	return projDb.MemberCount () == 1 && _state.IsAdmin ();
}

bool Permissions::ForceToObserver () const
{
	if (HasValidLicense () || IsReceiver ())
		return false;

#if !defined (BETA)
	// Release or debug build -- check trial
	return !IsTrial ();
#else
	// Beta build -- during beta we have unlimited trial
	return false;
#endif
}

bool Permissions::NeedsLocalLicenseUpdate () const
{
	if (IsProjectDataValid ())
	{
		if (_state.IsReceiver ())
			return false;	// Receiver never needs local license update

		// If the global license has different licensee name then the local license
		// or the global license has more seats for the same licensee then
		// we have to update the local license.
		return IsChangedLicensee (_local) || IsChangedProduct (_local) || _global.IsBetterThan (_local);
	}
	return false;
}

bool Permissions::IsBetterOrDifferentLicense (License const & license) const
{
	// If the new license has different licensee name or product type then the global license
	// or the new license has more seats for the same licensee then
	// we have to update the global license.
	// Check for condition -- more seats for this licensee or the new licensee
	return IsChangedLicensee (license) || IsChangedProduct (license) || license.IsBetterThan (_global);
}

std::string Permissions::GetLicenseString () const
{
	if (IsProjectDataValid ())
	{
		if (_state.IsReceiver ())
		{
			if (_local.IsCurrentVersion ())
				return _local.GetLicenseString ();
		}
	}

	if (_global.IsCurrentVersion ())
	{
		return _global.GetLicenseString ();
	}
	return std::string ();
}

std::string Permissions::GetProjectLicenseString () const
{
	Assert (IsProjectDataValid ());
	return _local.GetLicenseString ();
}

std::string Permissions::GetNewProjectLicense () const
{
	return _global.GetLicenseString ();
}

std::string Permissions::GetBranchLicense () const
{
	Assert (IsProjectDataValid ());
	Assert (_state.IsReceiver ());
	if (_state.NoBranching ())
		return _global.GetLicenseString ();
	else
		return _local.GetLicenseString ();
}
