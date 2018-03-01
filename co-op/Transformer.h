#if !defined (TRANSFORMER_H)
#define TRANSFORMER_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "FileState.h"
#include "GlobalId.h"
#include "FileData.h"
#include "ScriptCmd.h"

class PathFinder;
class TransactionFileList;
class Path;
class XPhysicalFile;
class DataBase;
namespace Progress { class Meter; }
class CommandList;
class SynchArea;

class Transformer
{
public:
	// Post condition for all constructors:
	//	FileData exisits in the file datbase after transformer construction
    Transformer (DataBase & dataBase, UniqueName const & uname, FileType type);
    Transformer (DataBase & dataBase, GlobalId gid);
    Transformer (DataBase & dataBase, PathFinder & pathFinder, TransactionFileList & fileList, FileData const & fileData);

	void Init (FileData const & fileData);
    FileState GetState () const { return _state; }
    FileType  GetFileType () const { return _fileData->GetType (); }
    GlobalId GetGlobalId () const { return _fileData->GetGlobalId (); }
    char const * GetName () const { return _fileData->GetName ().c_str (); }
    UniqueName const & GetUniqueName () const { return _fileData->GetUniqueName (); }

    bool IsStateNone () const { return _state.IsNone (); }
    bool IsCheckedOut () const { return _state.IsRelevantIn (Area::Original); }
	bool IsSynchedOut () const { return _state.IsRelevantIn (Area::Synch); }
    bool IsFolder () const { return _fileData->GetType ().IsFolder (); }
	bool IsRoot () const { return _fileData->GetType ().IsRoot (); }
	bool IsRecoverable () const { return _fileData->GetType ().IsRecoverable (); }
	bool IsRenamedIn (Area::Location loc) const { return _fileData->IsRenamedIn (loc); }
	UniqueName const & GetOriginalUname () const;
	UniqueName const & GetUnameIn (Area::Location loc) const  { return _fileData->GetUnameIn (loc); }
	GlobalId IsPotentialNameConflict (Area::Location areaFrom) const;
	GlobalId IsNameConflict (Area::Location areaFrom) const;
	void ListMovedFiles (GidList & files) const;

    void AddFile (PathFinder & pathFinder, TransactionFileList & fileList);
	void ReviveToBeDeleted ();
    void CheckOutIfNecessary (PathFinder & pathFinder, TransactionFileList & fileList);
    bool CheckOut (PathFinder & pathFinder,
				   TransactionFileList & fileList,
				   bool verifyChecksum = true,
				   bool ignoreChecksum = false);
    bool DeleteFile (PathFinder & pathFinder, TransactionFileList & fileList, bool doDelete);
    void MoveFile (PathFinder & pathFinder, TransactionFileList & fileList, UniqueName const & newName);
	void ChangeFileType (PathFinder & pathFinder, TransactionFileList & fileList, const FileType & fileType);
    bool Uncheckout (PathFinder & pathFinder, TransactionFileList & fileList, bool isVirtual = false);
    bool CheckIn (PathFinder & pathFinder, TransactionFileList & fileList, CommandList & cmdList);
    void CreateRelevantReferenceIfNecessary (PathFinder & pathFinder, TransactionFileList & fileList);
    void CreateUnrelevantReference (PathFinder & pathFinder, TransactionFileList & fileList);
    void CleanupReferenceIfPossible (PathFinder & pathFinder, TransactionFileList & fileList);
    bool CopyReference2Synch (PathFinder & pathFinder, TransactionFileList & fileList);
    void Merge (PathFinder & pathFinder, 
				TransactionFileList & fileList, 
				SynchArea & synchArea, 
				Progress::Meter & meter);
    bool UnpackDelete (PathFinder & pathFinder,
					   TransactionFileList & fileList,
					   Area::Location area,
					   bool doDelete);
	bool UnpackNew (Area::Location targetArea);
    void AcceptSynch (PathFinder & pathFinder, TransactionFileList & fileList, SynchArea & synchArea);
	void RemoveFromReference ();
	void CopyToReference (PathFinder & pathFinder, TransactionFileList & fileList);
	void UndoRename (UniqueName const & originalName);
	void UndoChangeType (FileType const & oldType);
	void MakeProjectFolder ();
	void MakeCheckedIn (PathFinder & pathFinder,
						TransactionFileList & fileList,
						Area::Location areaFrom = Area::Staging);
	void MakeNotInProject (bool recursive = true);
	void MakeNotInProjectFolderContents ();
	void MakeInArea (Area::Location area);
	void RenameIn (Area::Location loc, UniqueName const & uname);
	void ChangeTypeIn (Area::Location loc, FileType const & type);
    void MoveReference2Project (PathFinder & pathFinder, TransactionFileList & fileList);
	CheckSum GetCheckSum (Area::Location loc, XPhysicalFile & file);
    void ResolveNameConflict (PathFinder & pathFinder, TransactionFileList & fileList, bool isVirtual);
	void CopyToProject (Area::Location areaFrom, 
						XPhysicalFile & file, 
						PathFinder & pathFinder,
						TransactionFileList & fileList);
	void CopyAlias (Area::Location fromArea, Area::Location toArea);
	void SetMergeConflict (bool flag);

private:
	// Primitives
	void SetState (FileState state) { _fileData->SetState (state); }
	void DeleteFrom (Area::Location loc, XPhysicalFile & file);
	void CopyOverToProject (Area::Location fromArea,  UniqueName const & newName, XPhysicalFile & file);
	void Copy (Area::Location fromArea, Area::Location toArea, XPhysicalFile & file);

	void MergeContents (XPhysicalFile & file,
						TransactionFileList & fileList,
						PathFinder & pathFinder,
						Progress::Meter & meter);
    void MergeFolder (PathFinder & pathFinder, TransactionFileList & fileList);
    void MergeFile (PathFinder & pathFinder, TransactionFileList & fileList, SynchArea const & synchArea, Progress::Meter & meter);
    void CheckInFolder (PathFinder & pathFinder, TransactionFileList & fileList, CommandList & cmdList);
    void CheckInFile (PathFinder & pathFinder, TransactionFileList & fileList, CommandList & cmdList);
    void VerifyRenameFile (FileData const & fileData) const;

private:
    DataBase &	_dataBase;
    FileData  * _fileData;
    FileState	_state;
};

#endif
