#if !defined (PERMISSIONS_H)
#define PERMISSIONS_H
//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "License.h"
#include "ProjectSeats.h"
#include "Trial.h"
#include "MemberInfo.h"

class Catalog;
class PathFinder;
namespace Project
{
	class Db;
}

class Permissions
{
public:
	Permissions (Catalog & catalog);

	bool IsProjectDataValid () const { return _projectDataValid; }

	void TestProjectLockConsistency (PathFinder & pathFinder);
	void SetLicense (Catalog & catalog, License const & newLicense);
	void RefreshProjectData (Project::Db const & projectDb, PathFinder & pathFinder);
	void XRefreshProjectData (Project::Db const & projectDb, PathFinder * pathFinder = 0);
	void PropagateLocalLicense (Catalog & catalog);

	void ClearProjectData ();

	bool HasValidLicense () const;
	bool IsGlobalLicenseeEmpty () const;
	bool CanCreateNewProject () const;
	bool CanCreateNewBranch () const;
	bool CanExecuteSetScripts () const;
	bool MayBecomeDistributor (Project::Db const & projDb) const;

	MemberState GetState () const
	{
		Assert (IsProjectDataValid ());
		return _state;
	}
	bool IsReceiver () const
	{
		return IsProjectDataValid () ? _state.IsReceiver () : false;
	}
	bool IsDistributor () const
	{
		return IsProjectDataValid () ? _state.IsDistributor () : false;
	}
	bool IsNoBranching () const
	{
		return IsProjectDataValid () ? _state.NoBranching () : false;
	}
	bool IsAdmin () const
	{
		return IsProjectDataValid () ? _state.IsAdmin () : false;
	}
	bool ForceToObserver () const;
	bool NeedsLocalLicenseUpdate () const;
	bool IsBetterOrDifferentLicense (License const & license) const; 
	bool IsChangedLicensee (License const & newLicense) const;
	bool IsChangedProduct (License const & newLicense) const;
	std::string GetLicenseString () const;
	std::string GetProjectLicenseString () const;
	std::string GetNewProjectLicense () const;
	std::string GetBranchLicense () const;

	bool IsTrial () const { return _trial.IsOn (); }
	unsigned int GetTrialDaysLeft () const { return _trial.GetDaysLeft (); }

private:
	License			_global;
	License			_local;
	Trial			_trial;
	Project::Seats	_seats;
	MemberState		_state;
	bool			_projectDataValid;
};

#endif
