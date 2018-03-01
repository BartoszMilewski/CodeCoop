#if !defined DATABASE_H
#define DATABASE_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "FileDb.h"
#include "ProjectDb.h"
#include "GlobalId.h"
#include "VerificationReport.h"
#include "XArray.h"
#include "XString.h"

class CommandList;
class ScriptList;
class Path;
class PathFinder;
namespace History
{
	class Db;
}
class Directory;
class Matcher;
class AddresseeList;
class MemberDescription;
namespace Progress
{
	class Meter;
}
class MemoryLog;

class FileIndex
{
	typedef std::map<UniqueName, GlobalId>::iterator iterator;
	typedef std::map<UniqueName, GlobalId>::const_iterator const_iterator;

public:
	FileIndex ()
		: _isValid (false)
	{}

	virtual FileData const * FindByGid (GlobalId gid) const = 0;
	// Throws when not found
	virtual FileData const * GetFileDataByGid  (GlobalId gid) const = 0;
	
	virtual FileData const * FindProjectFileByName (UniqueName const & uname) const = 0;

	virtual void ListFolderContents (GlobalId folderId, GidList & content, bool recursive) const = 0;

	virtual bool CanReviveFolder (UniqueName const & uname) const = 0;

protected:
	GlobalId GetGlobalId (UniqueName const & uname) const
	{
		const_iterator it = _ids.find (uname);
		return it != _ids.end ()? it->second: gidInvalid;
	}
	void Invalidate () { _isValid = false; }
	void Validate (FileDb::FileDataIter beg, FileDb::FileDataIter end) const;

protected:
	mutable std::map<UniqueName, GlobalId>	_ids;
	mutable bool							_isValid;
};

class DataBase : public FileIndex, public TransactableContainer
{
    friend class PathFinder;
    friend class SynchTransaction;

public:
    DataBase ()
    {
		AddTransactableMember (_fileDb);
		AddTransactableMember (_projectDb);
		AddTransactableMember (_orgId);
	}

    void InitPaths (PathFinder & pathFinder);
	void FindAllByName (Matcher const & matcher, GidList & fileList) const
	{
		_fileDb.FindAllByName (matcher, fileList);
	}
	bool CanReviveFolder (UniqueName const & uname) const;
	FileData const * FindProjectFileByName (UniqueName const & uname) const;
    FileData const * XFindProjectFileByName (UniqueName const & uname) const
    {
        return _fileDb.XFindProjectFileByName (uname);
    }
	FileData * XFindAbsentFolderByName (UniqueName const & uname)
	{
		return _fileDb.XFindAbsentFolderByName (uname);
	}
    FileData const * GetFileDataByGid (GlobalId fileGid) const
    {
        return _fileDb.SearchByGid (fileGid);
    }
	FileData const * GetParentFileData (FileData const * fileData) const
	{
		return _fileDb.SearchByGid (fileData->GetUniqueName ().GetParentId ());
	}
    FileData const * XGetFileDataByGid (GlobalId fileGid) const
    {
        return _fileDb.XSearchByGid (fileGid);
    }
	FileData const * XGetParentFileData (FileData const * fileData)
	{
		return _fileDb.XSearchByGid (fileData->GetUniqueName ().GetParentId ());
	}
    FileData const * FindByGid (GlobalId gid) const
    {
        return _fileDb.FindByGid (gid);
    }
    FileData const * XFindByGid (GlobalId gid) const
    {
        return _fileDb.XFindByGid (gid);
    }
	FileData * XGetEdit (GlobalId gid)
	{
		return _fileDb.XGetEdit (gid);
	}
	bool XFindInFolder (GlobalId  gidFolder, std::function<bool(long, long)> predicate) const
	{
		return _fileDb.XFindInFolder (gidFolder, predicate);
	}
	void FindAllDescendants (GlobalId gidFolder, GidList & gidList) const
	{
		_fileDb.FindAllDescendants (gidFolder, gidList);
	}
	// Iterators
	typedef FileDb::FileDataIter FileIter;
	FileIter begin () const { return _fileDb.begin (); }
	FileIter end () const { return _fileDb.end (); }
	FileIter xbegin () const { return _fileDb.xbegin (); }
	FileIter xend () const { return _fileDb.xend (); }
	int FileCount () const { return _fileDb.size (); }
    void XCreateProjectRoot (); 
    UserId GetMyId () const { return _projectDb.GetMyId (); }
	GlobalId GetNextScriptId () const { return _projectDb.GetNextScriptId (); }
    UserId XGetMyId () const { return _projectDb.XGetMyId (); }
	UserId GetAdminId () const { return _projectDb.GetAdminId (); }
	UserId XGetAdminId () const { return _projectDb.XGetAdminId (); }
	bool IsProjectAdmin () const { return _projectDb.IsProjectAdmin (); }
	bool XIsProjectAdmin () const { return _projectDb.XIsProjectAdmin (); }
	void XUpdateScriptCounter (GlobalId gid) { _projectDb.XUpdateScriptCounter (gid); }
	void XUpdateFileCounter (GlobalId gid) { _projectDb.XUpdateFileCounter (gid); }

    GlobalId GetRandomId () const { return _projectDb.RandomId (); }
    GlobalId GetRandomId (UserId userId) const { return _projectDb.RandomId (userId); }
    GlobalId GetRootId () const { return _fileDb.GetRootId (); }
	bool IsAutoSynch () const { return _projectDb.IsAutoSynch (); }
   	bool IsAutoFullSynch () const { return _projectDb.IsAutoFullSynch (); }
	bool IsAutoJoin () const { return _projectDb.IsAutoJoin (); }

	void ListCheckedOutFiles (GidList & ids) const { _fileDb.ListCheckedOutFiles (ids); }
	void XListCheckedOutFiles (GidList & ids) const { _fileDb.XListCheckedOutFiles (ids); }
    void ListSynchFiles (GidList & ids) const { _fileDb.ListSynchFiles (ids); }
    std::unique_ptr<VerificationReport> Verify (PathFinder & pathFinder, Progress::Meter & meter) const;
    void XListProjectFiles (GlobalId folderId, GidList & content) const { _fileDb.XListProjectFiles (folderId, content); }
    void ListProjectFiles (GlobalId folderId, GidList & content) const { _fileDb.ListProjectFiles (folderId, content); }
	void ListFolderContents (GlobalId folderId, GidList & contents, bool recursive = true) const;
    void XListAllFiles (GlobalId folderId, GidList & files) const { _fileDb.XListAllFiles (folderId, files); }

	void XRemoveKnownFilesFrom (GidSet & historicalFiles)
	{
		_fileDb.XRemoveKnownFilesFrom (historicalFiles);
	}
	void XImportFileData (std::vector<FileData>::const_iterator begin,
						  std::vector<FileData>::const_iterator end)
	{
		_fileDb.XImportFileData (begin, end);
	}

	Project::Db const & GetProjectDb () const { return _projectDb; }
	Project::Db & XGetProjectDb () { return _projectDb; }
	std::string const & GetCopyright () const { return _projectDb.GetCopyright (); }
    void XSetCopyright (std::string const & copyright) { _projectDb.XSetCopyright (copyright); }
    void XSetProjectName (std::string const & name) { _projectDb.XSetProjectName (name); }
	std::string const & ProjectName () const { return _projectDb.ProjectName (); }
	std::string const & XProjectName () const { return _projectDb.XProjectName (); }

    bool XScriptNeeded (bool isMemberChange = false) const { return _projectDb.XScriptNeeded (isMemberChange); }
    MemberState XGetMemberState (UserId userId) const { return _projectDb.XGetMemberState (userId); }
    MemberState GetMemberState (UserId userId) const { return _projectDb.GetMemberState (userId); }
    void XGetVotingList (GidList & ackList) const { _projectDb.XGetVotingList (ackList); }
    void XGetAllMemberList (GidList & memberList) const { _projectDb.XGetAllMemberList (memberList); }
    void GetAllMemberList (GidList & memberList) const { _projectDb.GetAllMemberList (memberList); }
    bool XAddMember (MemberInfo const & info)
    {
        return _projectDb.XAddMember (info);
    }
	void XReplaceDescription (UserId userId, MemberDescription const & newDescription)
	{
		_projectDb.XReplaceDescription (userId, newDescription);
	}
    void XUpdateProjectDb (MemberInfo const & update)
    {
        _projectDb.XUpdate (update);
    }
	void XChangeState (UserId userId, MemberState newState)
	{
		_projectDb.XChangeState (userId, newState);
	}

    bool XAreFilesOut () const { return _fileDb.XAreFilesOut (); }
    bool AreFilesOut () const { return _fileDb.AreFilesOut (); }
    GlobalId XIsUnique (GlobalId fileId, UniqueName const & uname) const
	{
		return _fileDb.XIsUnique (fileId, uname);
	}
	GlobalId XMakeScriptId () { return _projectDb.XMakeScriptId (); }
    void XDefect ();
    FileData * XAddFile (UniqueName const & uname, FileType type);
    FileData * XAddForeignFile (FileData const & fileData);
	FileData * XAddForeignFile (UniqueName const & uname, GlobalId gid, FileType type);
    void XAddForeignFolder (GlobalId gid, UniqueName const & uname);
    void XAddProjectFiles (PathFinder & pathFinder,
						   CommandList & initialFileInventory,
						   TmpProjectArea const & restoredFiles,
						   Progress::Meter & meter);
    int XGetOriginalId () const { return _orgId.XGetCurrent (); }
    int GetOriginalId () const { return _orgId.GetCurrent (); }
    std::unique_ptr<MemberDescription> RetrieveMemberDescription (UserId userId) const
    {
        return _projectDb.RetrieveMemberDescription (userId);
    }
    std::unique_ptr<MemberDescription> XRetrieveMemberDescription (UserId userId) const
    {
        return _projectDb.XRetrieveMemberDescription (userId);
    }
    std::vector<MemberInfo> RetrieveBroadcastList () const
	{
		return _projectDb.RetrieveBroadcastList ();
	}
	void XRetrieveBroadcastRecipients (AddresseeList & addressees) const
	{
		_projectDb.XRetrieveBroadcastRecipients (addressees);
	}
    void XRetrieveMulticastRecipients (GidSet const & filterOut,
									   AddresseeList & addressees) const
	{
		_projectDb.XRetrieveMulticastRecipients (filterOut, addressees);
	}
	std::unique_ptr<MemberInfo> RetrieveMemberInfo (UserId userId) const
	{
		return _projectDb.RetrieveMemberInfo (userId);
	}
	void VerifyLogs () const
	{
		_projectDb.VerifyLogs ();
	}
	void CopyLog (FilePath const & destPath) const
	{
		_projectDb.CopyLog (destPath);
	}

	// Conversion from version 4.2 to 4.5
	void XSetOldestAndMostRecentScriptIds (History::Db & history, Progress::Meter * meter, MemoryLog & log)
	{
		_projectDb.XSetOldestAndMostRecentScriptIds (history, meter, log);
	}

    // Transactable interface
	void CommitTransaction () throw ();
	void Clear () throw ();
    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);
    bool IsSection () const { return true; }
    int SectionId () const { return 'DATA'; }
    int VersionNo () const { return modelVersion; }

private:
    static void MissingFile (char const * fullPath);
    void    XSwitchOriginalAreaId () { _orgId.XSwitch (); }
    int     GetPrevOriginalId () const { return _orgId.GetPrevious (); }

private:
    FileDb				_fileDb;
	Project::Db			_projectDb;
    TransactableSwitch	_orgId;
};

#endif
