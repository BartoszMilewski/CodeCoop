#if !defined (CATALOG_H)
#define CATALOG_H
//-----------------------------------
// (c) Reliable Software 2000 -- 2006
//-----------------------------------

#include "GlobalDb.h"
#include "SysPath.h"
#include "Global.h"
#include "GlobalId.h"
#include "Transport.h"

#include <File/Path.h>
#include <iosfwd>

class ProjectUserData;
class MemberDescription;
class ClusterRecipient;
class License;

namespace Project
{
	class Data;
}
namespace XML { class Node; }

class Catalog
{
	friend class FileTypeSeq;
	friend class ClusterRecipSeq;
	friend class SimpleClusterRecipSeq;
	friend class ProjectSeq;
	friend class UserSeq;
	friend class HubListSeq;

public:
	class Tree
	{
	public:
		Tree (FilePath const & root)
			: _dbPath (root),
			  _inboxPath (root),
			  _publicInboxPath (root),
			  _updatesPath (root),
			  _logsPath (root),
			  _wikiPath (root)
		{
			_dbPath.DirDown ("Database");
			_inboxPath.DirDown ("Inbox");
			_publicInboxPath.DirDown ("PublicInbox");
			_updatesPath.DirDown ("Updates");
			_logsPath.DirDown ("Logs");
			_wikiPath.DirDown ("Wiki");
		}
		FilePath const & DatabasePath () const { return _dbPath; }
		FilePath const & InboxPath () const { return _inboxPath; }
		FilePath const & PublicInboxPath () const { return _publicInboxPath; }
		FilePath const & UpdatesPath () const { return _updatesPath; }
		FilePath const & LogsPath () const { return _logsPath; }
		FilePath const & WikiPath () const { return _wikiPath; }

	private:
		FilePath	_dbPath;
		FilePath	_inboxPath;
		FilePath	_publicInboxPath;
		FilePath	_updatesPath;
		FilePath	_logsPath;
		FilePath	_wikiPath;
	};

public:
	Catalog (bool create = false, char const * catPath = 0);
	void Flush ();

	static bool IsHubIdUnknown(std::string const & hubId)
	{
		return IsNocaseEqual (hubId, "Unknown");
	}
	FilePath const & GetLogsPath () const { return _catTree.LogsPath (); }
	FilePath const & GetWikiPath () const { return _catTree.WikiPath (); }
	// false if project already exists, assigns project id
	bool RememberNewProject (Project::Data & projData,
							 MemberDescription const & user);
    void ForgetProject (int projectId);
	void UndoNewProject (int newProjectId);
	void MoveProjectTree (int projId, FilePath const & newPath);
	void AddProject (int projId, std::string const & name, std::string const & path);
	void AddProjectMember (Address const & address, int projId);
	void AddRemovedProjectMember (Address const & address, int projId);
	// Project data by source path
    bool GetProjectData (std::string const & path, Project::Data & projData);
	// Project data by id
	void GetProjectData (int projId, Project::Data & projData);
	std::string GetProjectName (int projId);
	FilePath GetProjectSourcePath (int projId);
	FilePath GetProjectDataPath (int projId) const;
	FilePath GetProjectInboxPath (int projId) const;
	std::string GetUserId (int projId);
	std::string GetEncryptionPass (std::string const & projName);

	void RefreshUserId (int projectId, std::string const & userId);
	void RefreshProjMemberHubId (int projectId, std::string const & hubId);
	void RefreshUserData (Address const & address,
						std::string const & newHubId,
						std::string const & newUserId);

	// project bookkeeping
    bool IsSourcePathUsed (FilePath const & sourcePath, int & projId);
	bool IsPathInProject (FilePath const & path, int & projId);
    bool IsProjectNameUsed (std::string const & name);
	bool IsProjectUnavailable (int projectId);
	void MarkProjectUnavailable (int projectId);
	void MarkProjectAvailable(int projectId);

	// file types
	void AddFileType (std::string const & extension, FileType type);
	// licensing
	bool GetTrialStart (long & trialStart);
	void SetTrialStart (long trialStart);
	void SetLicense (License const & license);
	std::string const & GetLicensee ();
	std::string const & GetKey ();
	unsigned GetDistributorLicenseCount ();
	unsigned GetNextDistributorNumber ();
	std::string const & GetDistributorLicensee ();
	void SetDistributorLicense (std::string const & licensee, unsigned start, unsigned count);
	void RemoveDistributorLicense ();
	void ClearDistributorLicenses ();

	// ------------------
	// Dispatcher section
	// ------------------
	FilePath GetPublicInboxDir () const { return _catTree.PublicInboxPath (); }
	FilePath const & GetUpdatesPath () const { return _catTree.UpdatesPath (); }
	std::string GetHubId ();
	void SetHubId (std::string const & hubId);
	// how other cluster members talk to me
	Transport GetActiveIntraClusterTransportToMe ();
	Topology GetMyTopology ();
	bool HasHub (std::string & computerName);
	// void SetActiveIntraClusterTransportToMe (Transport const & newTransport);
	void GetIntraClusterTransportsToMe (TransportArray & transports);
	void SetIntraClusterTransportsToMe (TransportArray const & transports);
	// how I talk to my hub
	void GetTransportsToHub (TransportArray & transports);
    void SetTransportsToHub (TransportArray const & transports);
	// how I talk to other hubs
	void AddRemoteHub (std::string const & hubId, Transport const & transport);
	void DeleteRemoteHub (std::string const & hubId);
	Transport GetInterClusterTransport (std::string const & hubId);
	// how other hubs talk to me
	Transport GetHubRemoteActiveTransport ();

	void GetDispatcherSettings (Topology & topology,
								TransportArray & myTransports,
								TransportArray & hubTransports,
								TransportArray & hubRemoteTransports,
								std::string & hubId);
	void SetDispatcherSettings (Topology topology, 
								TransportArray const & myTransports,
								TransportArray const & hubTransports,
								TransportArray const & hubRemoteTransports,
								std::string const & hubId);

	bool PurgeRecipients (bool purgeLocal, bool purgeSatellite);
	void LocalHubIdChanged (std::string const & oldHubId, std::string const & newHubId);
	void KillLocalRecipient (Address const & address, int projectId);
	void KillRemovedLocalRecipient (Address const & address);
	void RemoveLocalUser (Address const & address);
	void AddClusterRecipient (Address const & address, Transport const & transport);
	void AddRemovedClusterRecipient (Address const & address, Transport const & transport);
	void RemoveClusterRecipient (Address const & address);
	void DeleteClusterRecipient (Address const & address);
	void ActivateClusterRecipient (Address const & address);
	void ChangeTransport (Address const & address, Transport const & newTransport);
	void ClearClusterRecipients ();
	
	void Dump (XML::Node * root, std::ostream & out, bool isDiagnostic = false);

private:
	int  FindFreeProjId ();

	bool FillInProjectUserData (int projectId, ProjectUserData & projData);
	void RefreshUserData (ProjectUserData const & projData, 
						  std::string const & newHubId, 
						  std::string const & newUserId);

private:
	Tree			_catTree;
	SysPathFinder	_pathFinder;
	GlobalDb		_globalDb;
};

class ProjectSeqImpl;

class ProjectSeq
{
public:
    ProjectSeq (Catalog & cat);
	~ProjectSeq ();

    bool AtEnd () const;
    void Advance ();
	bool SkipTo (int projId)
	{
		for (; !AtEnd (); Advance ())
		{
			if (GetProjectId () == projId)
				return true;
		}
		return false;
	}

	bool IsProjectUnavailable () const;
	int  GetProjectId () const;
    std::string GetProjectName ();
    FilePath GetProjectSourcePath ();
    FilePath GetProjectDataPath ();
	FilePath GetProjectInboxPath ();
	void FillInProjectData (Project::Data & projData);

private:
	std::unique_ptr<ProjectSeqImpl> _pImpl;
	Catalog::Tree const & _catTree;
};

class UserSeqImpl;

class UserSeq
{
public:
    UserSeq (Catalog & cat);
	~UserSeq ();
    bool AtEnd () const;
    void Advance ();
	bool SkipTo (int projId)
	{
		for (; !AtEnd (); Advance ())
		{
			if (GetProjectId () == projId && !IsRemoved ())
				return true;
		}
		return false;
	}
	int GetProjectId ();
	std::string GetUserId ();
	bool IsRemoved () const;
	void FillInProjectData (ProjectUserData & projData);

private:
	std::unique_ptr<UserSeqImpl> _pImpl;
	Catalog::Tree const & _catTree;
};

class FileTypeSeqImpl;

class FileTypeSeq
{
public:
	FileTypeSeq (Catalog & cat);
	~FileTypeSeq ();
	bool AtEnd () const;
	void Advance ();
	std::string GetExt ();
	FileType GetType ();
private:
	std::unique_ptr<FileTypeSeqImpl> _pImpl;
};

class ClusterRecipSeqImpl;

class ClusterRecipSeq
{
public:
	ClusterRecipSeq (Catalog & cat);
	~ClusterRecipSeq ();

	bool AtEnd () const;
	void Advance ();

	void GetClusterRecipient (ClusterRecipient & recip);

private:
	std::unique_ptr<ClusterRecipSeqImpl> _pImpl;
};

class SimpleClusterRecipSeq
{
public:
	SimpleClusterRecipSeq (Catalog & cat);
	~SimpleClusterRecipSeq ();

	bool AtEnd () const;
	void Advance ();

	void GetClusterRecipient (Address & address, Transport & transport, bool & isRemoved);

private:
	std::unique_ptr<ClusterRecipSeqImpl> _pImpl;
};

class HubListSeqImpl;

class HubListSeq
{
public:
    HubListSeq (Catalog & cat);
	~HubListSeq ();

    bool AtEnd () const;
    void Advance ();

	void GetHubEntry (std::string & hubId, Transport & transport);
private:
	std::unique_ptr<HubListSeqImpl> _pImpl;
};

#endif
