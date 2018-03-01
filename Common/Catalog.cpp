//-----------------------------------
// (c) Reliable Software 2000 -- 2006
//-----------------------------------

#include "precompiled.h"
#include "Catalog.h"
#include "MemberInfo.h"
#include "ProjectData.h"
#include "PathRegistry.h"
#include "GlobalDb.h"
#include "License.h"

#include <Ex/WinEx.h>
#include <StringOp.h>
#include <Dbg/Assert.h>
#include <File/Path.h>

ProjectSeq::ProjectSeq (Catalog & cat)
	: _pImpl (new ProjectSeqImpl (cat._globalDb, cat._pathFinder)),
	  _catTree (cat._catTree)
{}

FilePath ProjectSeq::GetProjectDataPath ()
{
	return _catTree.DatabasePath ().GetFilePath (ToString (GetProjectId ()));
}

FilePath ProjectSeq::GetProjectInboxPath ()
{
	return _catTree.InboxPath ().GetFilePath (ToString (GetProjectId ()));
}

void ProjectSeq::FillInProjectData (Project::Data & projData)
{
	_pImpl->FillInProjectData (projData);
	std::string projIdStr = ToString (GetProjectId ());
	projData.SetDataPath (_catTree.DatabasePath ().GetFilePath (projIdStr));
	projData.SetInboxPath (_catTree.InboxPath ().GetFilePath (projIdStr));
}


ProjectSeq::~ProjectSeq ()
{}

// forwarders

bool ProjectSeq::AtEnd () const 
{ 
	return _pImpl.get () == 0 || _pImpl->AtEnd (); 
}
void ProjectSeq::Advance () 
{ 
	_pImpl->Advance (); 
}
bool  ProjectSeq::IsProjectUnavailable () const 
{ 
	return _pImpl->IsProjectUnavailable (); 
}
int  ProjectSeq::GetProjectId () const 
{ 
	return _pImpl->GetProjectId (); 
}
std::string ProjectSeq::GetProjectName ()
{
	return _pImpl->GetProjectName ();
}

FilePath ProjectSeq::GetProjectSourcePath ()
{
	return _pImpl->GetProjectSourcePath ();
}

UserSeq::UserSeq (Catalog & cat)
	: _pImpl (new UserSeqImpl (cat._globalDb, cat._pathFinder)),
	  _catTree (cat._catTree)
{}

int UserSeq::GetProjectId ()
{
	return _pImpl->GetProjectId ();
}

std::string UserSeq::GetUserId ()
{
	return _pImpl->GetUserId ();
}

bool UserSeq::IsRemoved () const
{
	return _pImpl->IsRemoved ();
}

void UserSeq::FillInProjectData (ProjectUserData & projData)
{
	_pImpl->FillInProjectData (projData);
	std::string projIdStr = ToString (GetProjectId ());
	projData.SetDataPath (_catTree.DatabasePath ().GetFilePath (projIdStr));
	projData.SetInboxPath (_catTree.InboxPath ().GetFilePath (projIdStr));
}

UserSeq::~UserSeq ()
{}

// forwarders

bool UserSeq::AtEnd () const 
{ 
	return _pImpl.get () == 0 || _pImpl->AtEnd (); 
}
void UserSeq::Advance () 
{ 
	_pImpl->Advance (); 
}

//-------------------------------------

FileTypeSeq::FileTypeSeq (Catalog & cat)
	: _pImpl (new FileTypeSeqImpl (cat._globalDb, cat._pathFinder))
{}

FileTypeSeq::~FileTypeSeq () {}

// forwarders

bool FileTypeSeq::AtEnd () const 
{ 
	return _pImpl->AtEnd (); 
}
void FileTypeSeq::Advance () 
{ 
	_pImpl->Advance (); 
}


std::string FileTypeSeq::GetExt () 
{ 
	return _pImpl->GetExt (); 
}
FileType FileTypeSeq::GetType () 
{ 
	return _pImpl->GetType (); 
}

// Hub list sequencer

HubListSeq::HubListSeq (Catalog & cat)
	: _pImpl (new HubListSeqImpl (cat._globalDb, cat._pathFinder))
{}

HubListSeq::~HubListSeq ()
{
}

void HubListSeq::GetHubEntry (std::string & hubId, Transport & transport)
{
	return _pImpl->GetHubEntry (hubId, transport);
}

bool HubListSeq::AtEnd () const
{
	return _pImpl->AtEnd ();
}

void HubListSeq::Advance ()
{
	_pImpl->Advance ();
}

//----------
// Catalog
//----------

Catalog::Catalog (bool create, char const * catPath)
	: _catTree (catPath? catPath: Registry::GetCatalogPath ()),
	  _globalDb (_catTree.DatabasePath ())
{
	_pathFinder.Init (_catTree.DatabasePath ().GetDir ());
	if (!File::Exists (_pathFinder.GetSysPath ()))
		File::CreateFolder (_pathFinder.GetSysPath (), false);
	if (create && !File::Exists (_pathFinder.GetSwitchPath ()))
	{
		{
			SwitchFile file (_pathFinder); // create
		}
		// create all other db files
		Transaction xact (_globalDb, _pathFinder);
		// hub id must be initiated, for the catalog to be ready to work
		_globalDb.XSetHubId ("Unknown");
		xact.Commit ();
	}
	else
	{
		if (!File::Exists (_pathFinder.GetSwitchPath ()))
			throw Win::ExitException ("Cannot find global database: Run setup again.");
		// it will read the database in
		_globalDb.Refresh (_pathFinder);
	}
}

void Catalog::Flush ()
{
	CatTransaction xact (_globalDb, _pathFinder);
	xact.Commit ();
}

void Catalog::Dump (XML::Node * root, std::ostream & out, bool isDiagnostic)
{
	_globalDb.Refresh (_pathFinder);
	_globalDb.Dump (root, out, isDiagnostic);
}

std::string Catalog::GetHubId ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetHubId ();
}

Transport Catalog::GetInterClusterTransport (std::string const & hubId)
{
	for (HubListSeq seq (*this); !seq.AtEnd (); seq.Advance ())
	{
		Transport trans;
		std::string hid;
		seq.GetHubEntry (hid, trans);
		if (IsNocaseEqual (hubId, hid))
		{
			return trans;
		}
	}
	return Transport ();
}

Transport Catalog::GetHubRemoteActiveTransport ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetHubRemoteActiveTransport ();
}

void Catalog::AddFileType (std::string const & extension, FileType type)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XAddFileType (extension, type.GetValue ());
	xact.Commit ();
}

FilePath Catalog::GetProjectInboxPath (int projId) const
{
	return _catTree.InboxPath ().GetFilePath (ToString (projId));
}

bool Catalog::GetTrialStart (long & trialStart)
{
	_globalDb.Refresh (_pathFinder);
	trialStart = _globalDb.GetTrialStart ();
	return trialStart != 0;
}

void Catalog::SetTrialStart (long trialStart)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetTrialStart (trialStart);
	xact.Commit ();
}

void Catalog::SetLicense (License const & license)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetLicense (license.GetLicensee (), license.GetKey ());
	xact.Commit ();
}

std::string const & Catalog::GetLicensee ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetLicensee ();
}

unsigned Catalog::GetDistributorLicenseCount ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetDistributorLicenseCount ();
}

std::string const & Catalog::GetDistributorLicensee ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetDistributorLicensee ();
}

unsigned Catalog::GetNextDistributorNumber ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetNextDistributorNumber ();
}

void Catalog::SetDistributorLicense (std::string const & licensee, unsigned start, unsigned count)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XSetDistributorLicensee (licensee);
	_globalDb.XSetNextDistributorNumber (start);
	_globalDb.XSetDistributorLicenseCount (count);
	xact.Commit ();
}

void Catalog::RemoveDistributorLicense ()
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveDistributorLicense ();
	xact.Commit ();
}

void Catalog::ClearDistributorLicenses ()
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XClearDistributorLicenses ();
	xact.Commit ();
}

std::string const & Catalog::GetKey ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetKey ();
}

Transport Catalog::GetActiveIntraClusterTransportToMe ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetActiveIntraClusterTransportToMe ();
}

Topology Catalog::GetMyTopology ()
{
	_globalDb.Refresh (_pathFinder);
	return _globalDb.GetTopology ();
}

bool Catalog::HasHub (std::string & computerName)
{
	_globalDb.Refresh (_pathFinder);
	if (_globalDb.GetTopology ().HasHub ())
	{
		std::vector<Transport> myTransports;
		Transport::Method active; 
		_globalDb.GetIntraClusterTransportsToMe (myTransports, active);
		for (unsigned int i = 0; i < myTransports.size (); ++i)
		{
			if (myTransports [i].GetMethod () == Transport::Network)
			{
				FullPathSeq share (myTransports [i].GetRoute ().c_str ());
				computerName = share.GetServerName ();
				break;
			}
		}
		return true;
	}
	return false;
}

bool Catalog::GetProjectData (std::string const & path, Project::Data & projData)
{
	Assert (!path.empty ());
    for (ProjectSeq seq (*this); !seq.AtEnd (); seq.Advance ())
    {
		Project::Data proj;
		seq.FillInProjectData (proj);
        FilePath sourcePathFromCatalog (proj.GetRootDir ());
        if (sourcePathFromCatalog.IsEqualDir (path))
        {
			projData = proj;
            return true;
        }
    }
	return false;
}

void Catalog::GetProjectData (int projId, Project::Data & projData)
{
	ProjectSeq seq (*this);
	if (seq.SkipTo (projId))
	{
		seq.FillInProjectData (projData);
	}
}

std::string Catalog::GetProjectName (int projId)
{
	ProjectSeq seq (*this);
	if (seq.SkipTo (projId))
	{
		return seq.GetProjectName ();
	}
	return std::string ();
}

FilePath Catalog::GetProjectSourcePath (int projId)
{
	ProjectSeq seq (*this);
	if (seq.SkipTo (projId))
	{
		return seq.GetProjectSourcePath ();
	}
	return FilePath ();
}

FilePath Catalog::GetProjectDataPath (int projectId) const
{
	Assert (projectId != -1);
	return _catTree.DatabasePath ().GetFilePath (ToString (projectId));
}

void Catalog::RefreshUserId (int projectId, std::string const & newUserId)
{
	Assert (projectId != -1);
	ProjectUserData projData;
	if (FillInProjectUserData (projectId, projData))
	{
		if (!IsNocaseEqual (newUserId, projData.GetUserId ()))
		{
			RefreshUserData (projData, projData.GetHubId (), newUserId);
		}
	}
}

void Catalog::RefreshProjMemberHubId (int projectId, std::string const & hubId)
{
	Assert (projectId != -1);
	ProjectUserData projData;
	if (FillInProjectUserData (projectId, projData))
	{
		if (!IsNocaseEqual (hubId, projData.GetHubId ()))
		{
			RefreshUserData (projData, hubId, projData.GetUserId ());
		}
	}
}

void Catalog::RefreshUserData (Address const & address,
								std::string const & newHubId,
								std::string const & newUserId)
{
	Assert (!newHubId.empty ());
	Assert (!newUserId.empty ());

	CatTransaction xact (_globalDb, _pathFinder);
	bool permanently = IsNocaseEqual (address.GetHubId (), newHubId);
	_globalDb.XRemoveClusterRecipient (Address (newHubId, address.GetProjectName (), newUserId), true);
	_globalDb.XRefreshUserData (address, newHubId, newUserId, permanently);
	xact.Commit ();
}

bool Catalog::FillInProjectUserData (int projectId, ProjectUserData & projData)
{
	// seq locks access to database
	UserSeq seq (*this);
	seq.SkipTo (projectId);
	if (seq.AtEnd ())
		return false;

	seq.FillInProjectData (projData);
	return true;
}

std::string Catalog::GetUserId (int projectId)
{
	Assert (projectId != -1);
	UserSeq seq (*this);
	seq.SkipTo (projectId);
	if (seq.AtEnd ())
		return std::string ();
	return seq.GetUserId ();
}

std::string Catalog::GetEncryptionPass (std::string const & projName)
{
	// Revisit: implement
	return std::string ();
}

void Catalog::RemoveLocalUser (Address const & address)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveUser (address, false);
	xact.Commit ();
}

void Catalog::RefreshUserData (ProjectUserData const & projData, 
							   std::string const & newHubId, 
							   std::string const & newUserId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	bool permanent = IsNocaseEqual (newHubId, projData.GetHubId ());
	_globalDb.XRemoveUser (projData.GetAddress (), permanent);

	Address newAddress (newHubId, projData.GetProjectName (), newUserId);
	_globalDb.XRemoveClusterRecipient (newAddress, true);
	_globalDb.XAddUser (newAddress, projData.GetProjectId ());
	xact.Commit ();
}

void Catalog::LocalHubIdChanged (std::string const & oldHubId, std::string const & newHubId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XLocalHubIdChanged (oldHubId, newHubId);
	xact.Commit ();
}

bool Catalog::RememberNewProject (Project::Data & projData,
								  MemberDescription const & user)
{
	int projId;
	if (IsSourcePathUsed (projData.GetRootDir (), projId))
	{
		std::string name = GetProjectName (projId);
		if (IsNocaseEqual (name, projData.GetProjectName ()))
		{
			// fill out the missing data
			GetProjectData (projId, projData); 
			return false;
		}
		else
			throw Win::Exception ("There is a project with a different name at this location", 
									projData.GetRootDir ());
	}

	CatTransaction xact (_globalDb, _pathFinder);
	projId = _globalDb.XAddProject (projData.GetProjectName (), projData.GetRootDir ());
	projData.SetProjectId (projId);
	std::string projIdStr = ToString (projId);
	projData.SetDataPath (_catTree.DatabasePath ().GetFilePath (projIdStr));
	projData.SetInboxPath (_catTree.InboxPath ().GetFilePath (projIdStr));
	Address newAddress (user.GetHubId (), projData.GetProjectName (),  user.GetUserId ());
	_globalDb.XRemoveClusterRecipient (newAddress, true);
	_globalDb.XAddUser (newAddress, projId);
	xact.Commit ();
	return true;
}

void Catalog::AddProject (int projId, std::string const & name, std::string const & path)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XAddProject (projId, name, path);
	xact.Commit ();
}

void Catalog::AddProjectMember (Address const & address, int projId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveClusterRecipient (address, true);
	_globalDb.XAddUser (address, projId);
	xact.Commit ();
}

void Catalog::AddRemovedProjectMember (Address const & address, int projId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XRemoveClusterRecipient (address, true);
	_globalDb.XAddUser (address, projId);
	_globalDb.XRemoveUser (address, false); // remove, not kill
	xact.Commit ();
}

void Catalog::ForgetProject (int projectId)
{
	if (projectId != -1)
	{
		CatTransaction xact (_globalDb, _pathFinder);
		_globalDb.XRemoveProject (projectId);
		xact.Commit ();
	}
}

bool Catalog::IsSourcePathUsed (FilePath const & sourcePath, int & projId)
{
    for (ProjectSeq seq (*this); !seq.AtEnd (); seq.Advance ())
    {
		FilePath someProjectRootPath (seq.GetProjectSourcePath ());
        if (sourcePath.HasPrefix (someProjectRootPath) ||
			someProjectRootPath.HasPrefix (sourcePath))
        {
			projId = seq.GetProjectId ();
            return true;
        }
    }
	projId = -1;
	return false;
}

bool Catalog::IsPathInProject (FilePath const & path, int & projId)
{
    for (ProjectSeq seq (*this); !seq.AtEnd (); seq.Advance ())
    {
		FilePath someProjectRootPath (seq.GetProjectSourcePath ());
        if (path.HasPrefix (someProjectRootPath))
        {
			projId = seq.GetProjectId ();
            return true;
        }
    }
	projId = -1;
	return false;
}

bool Catalog::IsProjectNameUsed (std::string const & name)
{
    for (ProjectSeq seq (*this); !seq.AtEnd (); seq.Advance ())
    {
		std::string catalogName = seq.GetProjectName ();
        if (IsNocaseEqual (catalogName, name))
			return true;	// 'name' already present in the project catalog
    }
    return false;
}

bool Catalog::IsProjectUnavailable (int projectId)
{
	ProjectSeq seq (*this);
	seq.SkipTo (projectId);
	if (seq.AtEnd ())
		return false;

	return seq.IsProjectUnavailable ();
}

void Catalog::MarkProjectUnavailable (int projectId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XMarkProjectUnavailable (projectId);
	xact.Commit ();
}

void Catalog::MarkProjectAvailable (int projectId)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XMarkProjectAvailable (projectId);
	xact.Commit ();
}

void Catalog::UndoNewProject (int newProjId)
{
	ForgetProject (newProjId);
}

void Catalog::MoveProjectTree (int projId, FilePath const & newPath)
{
	CatTransaction xact (_globalDb, _pathFinder);
	_globalDb.XMoveProjectTree (projId, newPath.GetDir ());
	xact.Commit ();
}
// ---------------------------------
// Simplified cluster recipient sequencer
// For use outside of the Dispatcher
// ---------------------------------

SimpleClusterRecipSeq::SimpleClusterRecipSeq (Catalog & cat)
: _pImpl (new ClusterRecipSeqImpl (cat._globalDb, cat._pathFinder))
{}

void SimpleClusterRecipSeq::GetClusterRecipient (Address & address,
												 Transport & transport, 
												 bool & isRemoved)
{
	_pImpl->GetClusterRecipient (address, transport, isRemoved);
}

SimpleClusterRecipSeq::~SimpleClusterRecipSeq ()
{
}

bool SimpleClusterRecipSeq::AtEnd () const
{
	return _pImpl->AtEnd ();
}

void SimpleClusterRecipSeq::Advance ()
{
	_pImpl->Advance ();
}