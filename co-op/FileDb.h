#if !defined FILEDB_H
#define FILEDB_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "XArray.h"
#include "Transact.h"
#include "Params.h"
#include "FileData.h"

class PathFinder;
class CommandList;
class Matcher;
namespace Progress { class Meter; }
class TmpProjectArea;

class FileDb : public TransactableContainer
{
public:
	FileDb ()
	{
		AddTransactableMember (_fileData);
	}

	int size () const { return _fileData.Count (); }
	void ListCheckedOutFiles (GidList & ids) const;
	void XListCheckedOutFiles (GidList & ids) const;
	void ListSynchFiles (GidList & ids) const;
	FileData const * FindByGid (GlobalId gid) const;
	FileData const * XFindByGid (GlobalId gid) const;
	void XListProjectFiles (GlobalId folderId, GidList & contents) const;
	void ListProjectFiles (GlobalId folderId, GidList & contents) const;
	void FindMissingFromDisk (PathFinder & pathFinder, GidList & missing, Progress::Meter & meter) const;
	void XListAllFiles (GlobalId folderId, GidList & files) const;
	void XRemoveKnownFilesFrom (GidSet & historicalFiles);
	void XImportFileData (std::vector<FileData>::const_iterator begin,
						  std::vector<FileData>::const_iterator end);

	GlobalId GetRootId () const;

	FileData * XAppend (UniqueName const & uname, GlobalId gid, FileState state, FileType type);
	FileData * XAppendForeign (FileData const & fileData);

	FileData * XGetEdit (GlobalId gid);

	bool XAreFilesOut () const;
	bool AreFilesOut () const;
	GlobalId XIsUnique (GlobalId fileId, UniqueName const & uname) const;

	void XAddProjectFiles (PathFinder & pathFinder,
						   CommandList & initialFileInvnetory,
						   TmpProjectArea const & restoredFiles,
						   Progress::Meter & meter) const;
	FileData const * SearchByGid  (GlobalId gid) const;
	void FindAllByName (Matcher const & matcher, GidList & foundFile) const;
	void FindAllDescendants (GlobalId gidFolder, GidList & gidList) const;
	FileData const * FindAbsentFolderByName (UniqueName const & uname) const;

	// Search during transaction
	FileData const * XSearchByGid  (GlobalId gid) const;
	FileData const * XFindProjectFileByName (UniqueName const & uname) const;
	FileData * XFindAbsentFolderByName (UniqueName const & uname);
	bool XFindInFolder (GlobalId gidFolder, std::function<bool(long, long)> predicate) const;

	// Iterators
	typedef TransactableArray<FileData>::const_iterator FileDataIter;
	FileDataIter begin () const { return _fileData.begin (); }
	FileDataIter end () const { return _fileData.end (); }
	FileDataIter xbegin () const { return _fileData.xbegin (); }
	FileDataIter xend () const { return _fileData.xend (); }

	// Serializable
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
	bool IsSection () const { return true; }
	int  SectionId () const { return 'BASE'; }
	int  VersionNo () const { return modelVersion; }

private:
	void XAddFolders (CommandList & initialFileInvnetory, Progress::Meter & meter) const;
	void XAddChildren (GlobalId curParentId, 
					   std::vector<FileData const *> & folders, 
					   CommandList & initialFileInvnetory,
					   Progress::Meter & meter) const;
	static void XAddFile2FullSynch (FileData const * fileData,
							char const * path,
							CommandList & initialFileInvnetory,
							bool beLazy); // don't copy contents immediately

private:
	TransactableArray<FileData> _fileData;
};

#endif
