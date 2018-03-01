//------------------------------------
//  (c) Reliable Software, 2001 - 2008
//------------------------------------

#include "precompiled.h"
#include "Workspace.h"
#include "FileData.h"
#include "SelectIter.h"
#include "Database.h"
#include "Lineage.h"
#include "History.h"
#include "CmdLineSelection.h"
#include "FileTyper.h"
#include "OutputSink.h"
#include "HistoryFilter.h"
#include "FileChanges.h"
#include "HistoryRange.h"

#include <Ctrl/ProgressMeter.h>
#include <File/Path.h>
#include <StringOp.h>

#if defined (_DEBUG)
#include <iomanip>

std::ostream& operator<<(std::ostream& os, Workspace::Operation operation)
{
	os << "Operation: ";
	switch (operation)
	{
	case Workspace::Undefined:
		os << "undefined";
		break;
	case Workspace::Add:		// Add new name to the project name space
		os << "add";
		break;
	case Workspace::Move:		// Change name and/or position in the project tree
		os << "move";
		break;
	case Workspace::Edit:		// Change file contents
		os << "edit";
		break;
	case Workspace::Delete:		// Remove a name from the project name space and file from disk
		os << "delete";
		break;
	case Workspace::Remove:		// Remove a name from the project name space but leave file on disk
		os << "remove";
		break;
	case Workspace::Resolve:	// Current file/folder name cannot be used. Change it to “Previous …” name
		os << "resolve";
		break;
	case Workspace::Checkout:
		os << "checkout";
		break;
	case Workspace::Uncheckout:
		os << "un-checkout";
		break;
	case Workspace::Checkin:
		os << "check-in";
		break;
	case Workspace::Undo:
		os << "undo";
		break;
	case Workspace::Redo:
		os << "redo";
		break;
	case Workspace::Synch:
		os << "synch";
		break;
	default:
		os << "illegal";
		break;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, Workspace::Item const & item)
{
	os << item.GetOperation ();
	if (item.GetItemGid () != gidInvalid)
	{
		GlobalIdPack pack (item.GetItemGid ());
		os << "; " << pack.ToString ();
	}
	else
	{
		os << "; no gid yet";
	}
	if (item.HasEffectiveSource ())
	{
		UniqueName const & uname = item.GetEffectiveSource ();
		GlobalIdPack pack (uname.GetParentId ());
		os << "; Source: " << pack.ToSquaredString () << "\\" 
			<< uname.GetName ().c_str () << " - ";
		if (!item.IsSourceParentInSelection ())
			os << "not ";
		os << "in selection";
	}
	if (item.HasEffectiveTarget ())
	{
		os << "; Target: ";
		UniqueName const & uname = item.GetEffectiveTarget ();
		if (uname.GetParentId () != gidInvalid)
		{
			GlobalIdPack pack (uname.GetParentId ());
			os << pack.ToSquaredString () << "\\";
		}
		else
			os << "<gidInvalid>\\";
		os << uname.GetName ().c_str () << " - ";
		if (!item.IsTargetParentInSelection ())
			os << "not ";
		os << "in selection";
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, Workspace::Selection const & selection)
{
	if (selection.size () != 0 && selection.size () < 20)
	{
		unsigned int i = 0;
		for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
		{
			os << std::setw (3) << std::setfill (' ') << i << ": ";
			os << seq.GetItem ();
			os << std::endl;
			++i;
		}
	}
	else
	{
		os << "There are " << selection.size () << " items in the selection." << std::endl;
	}
	return os;
}

#endif

using namespace Workspace;

//
// WORKGROUP
//

XGroup::XGroup (DataBase const & dataBase, Selection & selection)
{
	GidList checkedOut;
	dataBase.XListCheckedOutFiles (checkedOut);
	// (sorted) set of selected items that are NOT new
	GidSet selectionSet;
	for (Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		Item const & item = seq.GetItem ();
		GlobalId gid = item.GetItemGid ();
		if (gid != gidInvalid)
			selectionSet.insert (gid);
	}

	// (sorted) list of checked out items
	GidSet checkedOutSet;
	for (GidList::const_iterator iter = checkedOut.begin (); iter != checkedOut.end (); ++iter)
		checkedOutSet.insert (*iter);

	// Working Set:
	// set of checked out items that are not in the selection
	GidList setDiff;
	std::set_difference (checkedOutSet.begin (), checkedOutSet.end (),
						 selectionSet.begin (), selectionSet.end (),
						 std::back_inserter (setDiff));

	// Remember FileData of working set items
	for (GidList::const_iterator iter = setDiff.begin (); iter != setDiff.end (); ++iter)
	{
		GlobalId gid = *iter;
		FileData const * fd = dataBase.XGetFileDataByGid (gid);
		_workgroup.push_back (fd);
	}
}

class IsUsingName : public std::unary_function<FileData const *, bool>
{
public:
	IsUsingName (UniqueName const & name)
		: _name (name)
	{}

	bool operator () (FileData const * fileData) const
	{
		UniqueName const & thisFileUname = fileData->GetUniqueName ();
		FileState state = fileData->GetState ();
		return !state.IsToBeDeleted () && thisFileUname.IsEqual (_name);
	}
private:
	UniqueName const & _name;
};

class WasUsingName : public std::unary_function<FileData const *, bool>
{
public:
	WasUsingName (UniqueName const & name)
		: _name (name)
	{}

	bool operator () (FileData const * fileData) const
	{
		if (fileData->IsRenamedIn (Area::Original))
		{
			// Renamed file was using _name
			UniqueName const & thisFileOldName = fileData->GetUnameIn (Area::Original);
			return thisFileOldName.IsEqual (_name);
		}
		else if (fileData->GetState ().IsToBeDeleted ())
		{
			// Deleted or removed file was using _name
			UniqueName const & thisFileOldName = fileData->GetUniqueName ();
			return thisFileOldName.IsEqual (_name);
		}
		return false;
	}
private:
	UniqueName const & _name;
};

class IsOrWasInFolder : public std::unary_function<FileData const *, bool>
{
public:
	IsOrWasInFolder (GlobalId gidFolder)
		: _gidFolder (gidFolder)
	{}

	bool operator () (FileData const * fileData) const
	{
		UniqueName const & thisFileUname = fileData->GetUniqueName ();
		if (thisFileUname.GetParentId () == _gidFolder)
			return true;	// Is in the folder
		if (fileData->IsRenamedIn (Area::Original))
		{
			UniqueName const & thisFileOldUname = fileData->GetUnameIn (Area::Original);
			if (thisFileOldUname.GetParentId () == _gidFolder)
				return true;	// Was in the folder
		}
		return false;
	}
private:
	GlobalId	_gidFolder;
};

FileData const * XGroup::IsUsing (UniqueName const & name) const
{
	Iterator iter = std::find_if (_workgroup.begin (),
								  _workgroup.end (),
								  IsUsingName (name));
	if (iter != _workgroup.end ())
		return *iter;
	else
		return 0;
}

FileData const * XGroup::WasUsing (UniqueName const & name) const
{
	Iterator iter = std::find_if (_workgroup.begin (),
								  _workgroup.end (),
								  WasUsingName (name));
	if (iter != _workgroup.end ())
		return *iter;
	else
		return 0;
}

FileData const * XGroup::IsFolderContentsPresent (GlobalId gidFolder) const
{
	Iterator iter = std::find_if (_workgroup.begin (),
								  _workgroup.end (),
								  IsOrWasInFolder (gidFolder));
	if (iter != _workgroup.end ())
		return *iter;
	else
		return 0;
}

//////////////////////////////////////////////////////////////////
//
// Existing Item
//

void Item::GetNonContentsFileChanges (FileChanges & changes) const
{
	if (HasBeenMoved ())
	{
		UniqueName const & target = GetEffectiveTarget ();
		UniqueName const & source = GetEffectiveSource ();
		if (source.GetParentId () != target.GetParentId ())
		{
			// Moved
			changes.SetLocalMove (true);
		}
		else
		{
			// Renamed
			changes.SetLocalRename (true);
		}
	}
	changes.SetLocalTypeChange (HasBeenChangedType ());
	changes.SetLocalDelete (!HasEffectiveTarget ());
	changes.SetLocalNew (!HasEffectiveSource ());
}

std::string Item::NormalizeTargetName (UniqueName const & uname)
{
	if (uname.IsNormalized ())
	{
		return uname.GetName ();
	}
	else
	{
		// Unique name is not normalized -- <gid>\FolderA\FolderB\FileName
		// Extract name
		PathSplitter splitter (uname.GetName ());
		std::string targetName (splitter.GetFileName ());
		targetName += splitter.GetExtension ();
		return targetName;
	}
}

// Effective source and target are reversed for Uncheckout

std::string const & ExistingItem::GetEffectiveTargetName () const
{
	UniqueName const & targetUname = GetEffectiveTarget ();
	return targetUname.GetName ();
}

FileType ExistingItem::GetEffectiveTargetType () const
{
	if (_targetType.IsInvalid ())
	{
		Assert (_fileData != 0);
		return _fileData->GetType ();
	}
	return _targetType;
}

GlobalId ExistingItem::GetItemGid () const
{
	Assert (_fileData != 0);
	return _fileData->GetGlobalId ();
}

UniqueName const & ExistingItem::GetEffectiveTarget () const
{
	Assert (HasEffectiveTarget ());
	Operation operation = GetOperation ();
	Assert (operation != Undefined);
	if (operation == Uncheckout)
		return GetIntendedSource ();
	else
		return GetIntendedTarget ();
}

UniqueName const & ExistingItem::GetIntendedTarget () const
{
	Assert (_fileData != 0);
	return _fileData->GetUniqueName ();
}

UniqueName const & ExistingItem::GetEffectiveSource () const
{
	Assert (HasEffectiveSource ());
	Operation operation = GetOperation ();
	Assert (operation != Undefined);
	if (operation == Uncheckout)
		return GetIntendedTarget ();
	else
		return GetIntendedSource ();
}

UniqueName const & ExistingItem::GetIntendedSource () const
{
	Assert (_fileData != 0);
	if (_fileData->IsRenamedIn (Area::Original))
		return _fileData->GetUnameIn (Area::Original);

	return _fileData->GetUniqueName ();
}

FileType ExistingItem::GetEffectiveSourceType () const  
{
	Assert (_fileData != 0);
	if (_fileData->IsTypeChangeIn (Area::Original))
		return _fileData->GetTypeIn (Area::Original);
	return _fileData->GetType ();
}

bool ExistingItem::HasIntendedSource () const
{
	Assert (_fileData != 0);
	return _fileData->GetState ().IsNew () ? false : true;
}

bool ExistingItem::HasEffectiveSource () const
{
	Assert (_fileData != 0);
	Operation operation = GetOperation ();
	if (operation == Uncheckout)
		return _fileData->GetState ().IsToBeDeleted () ? false : true;
	return _fileData->GetState ().IsNew () ? false : true;
}

bool ExistingItem::HasEffectiveTarget () const
{
	Assert (_fileData != 0);
	Operation operation = GetOperation ();
	if (operation == Delete || operation == Remove)
		return false;
	else if (operation == Uncheckout)
		return _fileData->GetState ().IsNew () ? false : true;

	return _fileData->GetState ().IsToBeDeleted () ? false : true;
}

bool ExistingItem::HasIntendedTarget () const
{
	Operation operation = GetOperation ();
	if (operation == Delete || operation == Remove)
		return false;
	return _fileData->GetState ().IsToBeDeleted () ? false : true;
}

bool ExistingItem::HasBeenMoved () const
{
	Assert (_fileData != 0);
	return _fileData->IsRenamedIn (Area::Original);
}

bool ExistingItem::HasBeenChangedType () const
{
	Assert (_fileData != 0);
	return _fileData->IsTypeChangeIn (Area::Original);
}
 
bool ExistingItem::IsFolder () const
{
	Assert (_fileData != 0);
	return _fileData->GetType ().IsFolder ();
}

bool ExistingItem::IsChanged (PathFinder & pathFinder) const
{
	Assert (_fileData != 0);
	FileState state = _fileData->GetState ();
	if (!state.IsRelevantIn (Area::Original))
		return false;

	char const * projectPath = pathFinder.GetFullPath (_fileData->GetUniqueName ());
	if (state.IsPresentIn (Area::Project) && !File::Exists (projectPath))
		throw Win::Exception ("Cannot detect edit changes, because file/folder is not present on disk", projectPath);

	if (state.IsNew () || state.IsToBeDeleted ())
		return true;

	Assert (state.IsPresentIn (Area::Project));
	if (!_fileData->GetType ().IsFolder ())
	{
		if (HasBeenMoved () || HasBeenChangedType ())
			return true;
		std::string originalPath (pathFinder.GetFullPath (_fileData->GetGlobalId (), Area::Original));
		return !File::IsContentsEqual (originalPath.c_str (), projectPath);
	}
	return false;
}

bool ExistingItem::IsRecoverable () const
{
	Assert (_fileData != 0);
	return _fileData->GetType ().IsRecoverable ();
}

/////////////////////////////////////////////////////////////
//
// ToBeAdded Item
//

ToBeAddedItem::ToBeAddedItem (UniqueName const * uname, FileType fileType)
	: Item (Add),
	  _itemGid (gidInvalid),
	  _targetType (fileType),
	  _targetName (NormalizeTargetName (*uname)),
	  _targetUname (*uname)
{}

ToBeAddedItem::ToBeAddedItem (UniqueName const & uname, Item const * targetFolder)
	: Item (Add),
	  _itemGid (gidInvalid),
	  _targetType (FolderType ()),
	  _targetName (NormalizeTargetName (uname)),
	  _targetUname (uname)
{
	SetTargetParentItem (targetFolder);
}

/////////////////////////////////////////////////////////////
//
// ToBeMoved Item
//

ToBeMovedItem::ToBeMovedItem (FileData const * fd, UniqueName const & targetUname)
	: ExistingItem (fd, Move),
	  _targetName (NormalizeTargetName (targetUname)),
	  _targetUname (targetUname)
{}

/////////////////////////////////////////////////////////////
//
// Script Item
//

ScriptItem::ScriptItem (FileCmd const & cmd)
	: ExistingItem (&cmd.GetFileData (), Synch),
	  _fileCmd (cmd)
{}

bool ScriptItem::HasEffectiveSource () const
{
	return HasIntendedSource ();
}

bool ScriptItem::HasEffectiveTarget () const
{
	return HasIntendedTarget ();
}

bool ScriptItem::HasIntendedSource () const
{
	return (_fileCmd.GetSynchKind () == synchNew) ? false : true;
}

bool ScriptItem::HasIntendedTarget () const
{
	return (_fileCmd.GetSynchKind () == synchDelete ||
			_fileCmd.GetSynchKind () == synchRemove) ? false : true;
}

/////////////////////////////////////////////////////////////
//
// History Item
//

bool HistoryItem::HasEffectiveSource () const
{
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	FileCmd const * cmd;
	if (operation == Undo)
		cmd = _editCmds.front ();
	else
		cmd = _editCmds.back ();
	Assert (cmd != 0);
	ScriptCmdType cmdType = cmd->GetType ();
	if (operation == Undo)
		return (cmdType != typeDeletedFile) && (cmdType != typeDeleteFolder);
	else
		return (cmdType != typeWholeFile) && (cmdType != typeNewFolder);
}

bool HistoryItem::HasEffectiveTarget () const
{
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	FileCmd const * cmd;
	if (operation == Undo)
		cmd = _editCmds.back ();
	else
		cmd = _editCmds.front ();
	Assert (cmd != 0);
	ScriptCmdType cmdType = cmd->GetType ();
	if (operation == Undo)
		return (cmdType != typeWholeFile) && (cmdType != typeNewFolder);
	else
		return (cmdType != typeDeletedFile) && (cmdType != typeDeleteFolder);
}

bool HistoryItem::HasIntendedSource () const
{
	FileCmd const * cmd = _editCmds.back ();
	Assert (cmd != 0);
	ScriptCmdType cmdType = cmd->GetType ();
	return (cmdType != typeWholeFile) && (cmdType != typeNewFolder);
}

bool HistoryItem::HasIntendedTarget () const
{
	FileCmd const * cmd = _editCmds.front ();
	Assert (cmd != 0);
	ScriptCmdType cmdType = cmd->GetType ();
	return (cmdType != typeDeletedFile) && (cmdType != typeDeleteFolder);
}

bool HistoryItem::HasBeenMoved () const
{
	FileData const & tgtFd = GetOriginalTargetFileData ();
	if (_editCmds.size () == 1)
	{
		// Only one editing command
		return tgtFd.IsRenamedIn (Area::Original);
	}
	else
	{
		FileData const & srcFd = GetOriginalSourceFileData ();
		return !srcFd.GetUniqueName ().IsEqual (tgtFd.GetUniqueName ());
	}
}

bool HistoryItem::HasBeenChangedType () const
{
	if (_editCmds.size () == 1)
	{
		// Only one editing command
		FileData const & fd = GetOriginalTargetFileData ();
		return fd.IsTypeChangeIn (Area::Original);
	}
	else
	{
		FileType const & mostRecentType = GetEffectiveTargetType ();
		FileType const & oldestType = GetEffectiveSourceType ();
		return !mostRecentType.IsEqual (oldestType);
	}
}

bool HistoryItem::IsFolder () const
{
	FileData const & fd = GetOriginalSourceFileData ();
	return fd.GetType ().IsFolder ();
}

bool HistoryItem::IsRecoverable () const
{
	FileData const & srcFd = GetOriginalSourceFileData ();
	FileData const & tgtFd = GetOriginalTargetFileData ();
	return srcFd.GetType ().IsRecoverable () &&
		   tgtFd.GetType ().IsRecoverable ();
}

std::string const & HistoryItem::GetEffectiveTargetName () const
{
	UniqueName const & targetUname = GetEffectiveTarget ();
	return targetUname.GetName ();
}

FileType HistoryItem::GetEffectiveTargetType () const
{
	FileData const & fd = GetEffectiveTargetFileData ();
	return fd.GetType ();
}

FileType  HistoryItem::GetEffectiveSourceType () const
{
	FileData const & fd = GetEffectiveSourceFileData ();
	if (fd.IsTypeChangeIn (Area::Original))
		return fd.GetTypeIn (Area::Original);
	return fd.GetType ();
}

UniqueName const & HistoryItem::GetEffectiveTarget () const
{
	Assert (HasEffectiveTarget ());
	FileData const & fd = GetEffectiveTargetFileData ();
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	if (operation == Undo)
	{
		if (fd.IsRenamedIn (Area::Original))
			return fd.GetUnameIn (Area::Original);
	}
	return fd.GetUniqueName ();
}

UniqueName const & HistoryItem::GetIntendedTarget () const
{
	FileData const & fd = GetOriginalTargetFileData ();
	return fd.GetUniqueName ();
}

UniqueName const & HistoryItem::GetIntendedSource () const
{
	FileData const & fd = GetOriginalSourceFileData ();
	if (fd.IsRenamedIn (Area::Original))
		return fd.GetUnameIn (Area::Original);

	return fd.GetUniqueName ();
}

GlobalId HistoryItem::GetItemGid () const
{
	FileData const & fd = GetOriginalTargetFileData ();
	return fd.GetGlobalId ();
}

UniqueName const & HistoryItem::GetEffectiveSource () const
{
	Assert (HasEffectiveSource ());
	FileData const & fd = GetEffectiveSourceFileData ();
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	if (operation == Redo)
	{
		if (fd.IsRenamedIn (Area::Original))
			return fd.GetUnameIn (Area::Original);
	}
	return fd.GetUniqueName ();
}

FileData const & HistoryItem::GetOriginalTargetFileData () const
{
	// Original target file data is defined by the most recent edit command
	FileCmd const * cmd = _editCmds.front ();
	Assert (cmd != 0);
	return cmd->GetFileData ();
}

FileData const & HistoryItem::GetOriginalSourceFileData () const
{
	// Original source file data is defined by the oldest edit command
	FileCmd const * cmd = _editCmds.back ();
	Assert (cmd != 0);
	return cmd->GetFileData ();
}

FileData const & HistoryItem::GetEffectiveTargetFileData () const
{
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	if (operation == Undo)
		return GetOriginalSourceFileData ();
	else
		return GetOriginalTargetFileData ();
}

FileData const & HistoryItem::GetEffectiveSourceFileData () const
{
	Operation operation = GetOperation ();
	Assert (operation == Redo || operation == Undo);
	if (operation == Undo)
		return GetOriginalTargetFileData ();
	else
		return GetOriginalSourceFileData ();
}

std::string const & HistoryItem::GetLastName () const
{
	FileData const & lastFileData = GetEffectiveTargetFileData ();
	return lastFileData.GetName ();
}

//////////////////////////////////////////////////////////////////////
//
// Selection
//

void Selection::ItemVector::push_back (std::unique_ptr<Item> item)
{
	GlobalId gid = item->GetItemGid ();
	if (gid != gidInvalid)
	{
		if (item->IsFolder ())
		{
			Assert (_folderGid2Item.find (gid) == _folderGid2Item.end ());
			_folderGid2Item [gid] = item.get ();
		}
		if (_itemSet.find (gid) != _itemSet.end ())
			throw Win::Exception ("Failed to execute a corrupted script");
        _itemSet.insert (gid);
	}
	_items.push_back (std::move(item));
}

#if 0
void Selection::ItemVector::erase (unsigned int i)
{
	Item const * item = operator [](i);
	GlobalId gid = item->GetItemGid ();
	if (gid != gidInvalid)
	{
		if (item->IsFolder ())
			_folderGid2Item.erase (gid);
		_itemSet.erase (gid);
	}
	_items.erase (i);
}
#endif

void Selection::ItemVector::Erase(GidSet const & eraseSet)
{
	auto_vector<Item>::iterator it = _items.begin();
	while (it != _items.end())
	{
		if (eraseSet.find((*it)->GetItemGid()) != eraseSet.end())
			it = _items.erase(it);
		else
			++it;
	}
	for (GidSet::const_iterator setIt = eraseSet.begin(); setIt != eraseSet.end(); ++setIt)
		_itemSet.erase(*setIt);
}

class IsEqualId : public std::unary_function<Item const *, bool>
{
public:
	IsEqualId (GlobalId gid)
		: _gid (gid)
	{}
	bool operator () (Item const * item) const
	{
		return item->GetItemGid () == _gid;
	}

private:
	GlobalId	_gid;
};

Item const & Selection::ItemVector::Find (GlobalId gid) const
{
	Assert (IsIncluded (gid));
	Iterator iter = std::find_if (begin (), end (), IsEqualId (gid));
	Assert (iter != end ());
	return **iter;
}

char const * Selection::_incompleteOriginalSelection = "Incomplete original selection";

// Returns pointer to the folder item in the selection or 0
Item const * Selection::ItemVector::IsFolderIncluded (GlobalId folderGid) const
{
	typedef std::map<GlobalId, Item const *>::const_iterator FolderIter;

	FolderIter iter = _folderGid2Item.find (folderGid);
	if (iter != _folderGid2Item.end ())
		return iter->second;
	return 0;
}

// Forward -- used during check-out, change file type
Selection::Selection (GidList const & files, DataBase const & dataBase, Operation operation)
{
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		Assert (gid != gidInvalid);
		FileData const * fd = dataBase.GetFileDataByGid (gid);
		if (!fd->GetState ().IsNone ())
		{
			std::unique_ptr<Item> item (new ExistingItem (fd, operation));
			_items.push_back (std::move(item));
		}
	}
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

// Used during rename
Selection::Selection (GidList const & files,
					  DataBase const & dataBase,
					  UniqueName const & targetName)
{
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		GlobalId gid = *iter;
		Assert (gid != gidInvalid);
		FileData const * fd = dataBase.GetFileDataByGid (gid);
		Assert (!fd->GetState ().IsNone ());
		std::unique_ptr<Item> item (new ToBeMovedItem (fd, targetName));
		_items.push_back(std::move(item));
	}
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

// Used during paste of controlled files
Selection::Selection (auto_vector<FileTag> const & fileClipboard,
					  DataBase const & dataBase,
					  Directory const & folder)
{
	GlobalId targetFolderId = folder.GetCurrentId ();
	for (auto_vector<FileTag>::const_iterator iter = fileClipboard.begin ();
		 iter != fileClipboard.end ();
		 ++iter)
	{
		GlobalId gid = (*iter)->Gid ();
		if (gid != gidInvalid)
		{
			FileData const * fd = dataBase.GetFileDataByGid (gid);
			UniqueName const & currentName = fd->GetUniqueName ();
			if (currentName.GetParentId () == targetFolderId)
				continue;	// Cut/Paste in the same folder not supported
			UniqueName targetName;
			if (targetFolderId == gidInvalid)
			{
				// Paste in the not controlled folder
				PathSplitter splitter ((*iter)->Path ());
				std::string fileName (splitter.GetFileName ());
				fileName += splitter.GetExtension ();
				PathParser parser (folder);
				char const * targetPath = folder.GetFilePath (fileName.c_str ());
				UniqueName const * targetUname = parser.Convert (targetPath);
				Assert (targetUname != 0);
				targetName.Init (*targetUname);
			}
			else
			{
				// Paste in the controlled folder
				targetName.Init (targetFolderId, currentName.GetName ().c_str ());
			}
			std::unique_ptr<Item> item (new ToBeMovedItem (fd, targetName));
			_items.push_back (std::move(item));
		}
	}
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

// Used during adding new files to the project -- Project>New or file drop
Selection::Selection (std::vector<std::string> const & files, PathParser & pathParser)
{
	// Convert full paths into unique names
	std::map<UniqueName, Item const *> addedNonProjectFolders;
	// Add folders first
    for (std::vector<std::string>::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
		std::string const & path = *iter;
		if (File::IsFolder (path.c_str ()))
		{
			UniqueName const * targetUname = pathParser.Convert (path.c_str ());
			Assert (targetUname != 0);
			std::unique_ptr<Item> item (new ToBeAddedItem (targetUname, FolderType ()));
			_items.push_back (std::move(item));
			// Set default sequencing order for added folders
			_sortVector.push_back (_items.back ());
			addedNonProjectFolders [*targetUname] = _items.back ();
		}
	}

	// Now add files
    for (std::vector<std::string>::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
		std::string const & path = *iter;
		if (!File::IsFolder (path.c_str ()))
		{
			UniqueName const * targetUname = pathParser.Convert (path.c_str ());
			Assert (targetUname != 0);
			std::unique_ptr<Item> item (new ToBeAddedItem (targetUname));
			_items.push_back (std::move(item));
			Item * curItem = _items.back ();
			// Set default sequencing order for added files
			_sortVector.push_back (curItem);
			Item const * targetFolderItem = 0;
			if (targetUname->IsNormalized ())
			{
				targetFolderItem = _items.IsFolderIncluded (targetUname->GetParentId ());
			}
			else
			{
				// Parsing unique name will add necessary folders to the selection
				targetFolderItem = ParseUname (*targetUname, addedNonProjectFolders);
			}
			curItem->SetTargetParentItem (targetFolderItem);
		}
	}
	Assert (_items.size () == _sortVector.size ());
}

// Used during Folder>New File or Folder>New Folder
Selection::Selection (std::string const & path, PathParser & pathParser)
{
	// Convert full path into unique names
	UniqueName const * targetUname = pathParser.Convert (path.c_str ());
	if (targetUname == 0)
		return;
	std::unique_ptr<Item> item (new ToBeAddedItem (targetUname));
	_items.push_back (std::move(item));
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

void Selection::AddContents (DataBase const & dataBase, bool recursive)
{
	// We don't use iterator because items are added to the _items vector
	// and we don't want to iterate over added items
	unsigned int originalItemCount = _items.size ();
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item const * item = _items [i];
		if (item->IsFolder () && item->GetItemGid () != gidInvalid)
		{
			AddFolderContents (item, dataBase, recursive);
			LinkItems ();
		}
	}
}

void Selection::Extend (DataBase const & dataBase)
{
	// Notice: it's okay to store pointers that are kept in auto_vector
	std::map<UniqueName, Item const *> nonProjectFolders;
	unsigned int originalItemCount = _items.size ();
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item const * curItem = _items [i];
		if (curItem->IsFolder () && curItem->GetItemGid () == gidInvalid)
		{
			// Remember non-project folder
			UniqueName const & targetUname = curItem->GetEffectiveTarget ();
			nonProjectFolders [targetUname] = curItem;
		}
	}
	// We don't use iterator because items are added to the _items vector
	// and we don't want to iterate over added items
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item * curItem = _items [i];
		if (curItem->HasEffectiveTarget ())
		{
			if (curItem->IsTargetParentInSelection ())
				continue; // Item's target folder already in selection - nothing to do

			// The parent of the item's target is not in the selection. Should we add it?
			UniqueName const & targetUname = curItem->GetEffectiveTarget ();
			Item const * targetParent = 0;
			if (targetUname.IsNormalized ())
			{
				// Target folder already in the project -- check its state.
				// Checking target folder state may add folders to the selection.
				GlobalId targetParentGid = targetUname.GetParentId ();
				targetParent = AddFoldersIfNecessary (targetParentGid, dataBase);
			}
			else
			{
				// Target folder not in the project -- parse its unnormalized unique name
				// Parsing name will add folders to the selection
				targetParent = ParseUname (targetUname, nonProjectFolders);
			}
			curItem->SetTargetParentItem (targetParent);
		}
		else
		{
			// Item will be removed from project name space, if this is a folder
			// then its contents has to be also removed.
			Assert (curItem->GetItemGid () != gidInvalid);
			if (curItem->IsFolder ())
			{
				AddFolderContents (curItem, dataBase);
				LinkItems ();
			}
		}
	}
	Assert (_items.size () == _sortVector.size ());
}

void Selection::XMerge (XGroup const & workgroup)
{
	unsigned int originalItemCount = _items.size ();
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item * curItem = _items [i];
		Assert (curItem->GetOperation () != Undefined);
		if (curItem->HasEffectiveTarget ())
		{
			UniqueName const & targetUname = curItem->GetEffectiveTarget ();
			FileData const * conflict = workgroup.IsUsing (targetUname);
			if (conflict != 0)
			{
				// Workgroup item is using selection item's target name -- resolve name conflict
				Assert (conflict->GetState ().IsNew ()			|| // Local add or local restore delete
						conflict->IsRenamedIn (Area::Original)	|| // Local rename/move
						conflict->GetState ().IsSynchDelete ());   // Synch delete (directly or because of script conflict)
				AddResolveItem (conflict);
			}
			else
			{
				// No name conflict
				Operation operation = curItem->GetOperation ();
				if (operation == Undo)
				{
					// Undoing item changes
					// Don't overwrite file with the same name in the Project Area.
					// Rename in to "Previous ..."
					curItem->SetOverwrite (false);
				}
				if (operation == Uncheckout)
				{
					// Don't overwrite file with the same name in the Project Area
					// only if we are unchecking out file delete
					ExistingItem const * item = dynamic_cast<ExistingItem const *>(curItem);
					FileData const & fd = item->GetFileData ();
					FileState state = fd.GetState ();
					if (state.IsToBeDeleted () && state.IsCoDelete ())
					{
						// Rename in to "Previous ..."
						curItem->SetOverwrite (false);
					}
				}
			}
		}
	}
	Assert (_items.size () == _sortVector.size ());
}

class IsItemFolder : public std::unary_function<Item const *, bool>
{
public:
	bool operator () (Item const * item) const
	{
		return item->IsFolder ();
	}
};

void Selection::Sort ()
{
	ItemVector::Iterator iter = std::find_if (_items.begin (), _items.end (), IsItemFolder ());
	if (iter != _items.end ())
	{
		// Folders in the selection -- topologicaly sort selection
		TopologicalSorter sorter (_items);
		sorter.Sort (_sortVector);
	}
}

void Selection::SetOperation (Operation operation)
{
	for (ItemVector::Iterator iter = _items.begin ();
		 iter != _items.end ();
		 ++iter)
	{
		(*iter)->SetOperation (operation);
	}
}

void Selection::SetOverwrite (bool flag)
{
	for (ItemVector::Iterator iter = _items.begin ();
		 iter != _items.end ();
		 ++iter)
	{
		(*iter)->SetOverwrite (flag);
	}
}

void Selection::SetType (FileTyper & fileTyper, PathFinder & pathFinder)
{
	for (ItemVector::Iterator iter = _items.begin ();
		 iter != _items.end ();
		 ++iter)
	{
		FileType itemType = (*iter)->GetEffectiveTargetType ();
		if (itemType.IsInvalid ())
		{
			// File type not assigned
			char const * path = pathFinder.GetFullPath ((*iter)->GetEffectiveTarget ());
			FileType fileType;
			if (File::IsFolder (path))
			{
				fileType = FolderType ();
			}
			else
			{
				fileType = fileTyper.GetFileType ((*iter)->GetEffectiveTargetName ());
				if (fileType.IsAuto () || fileType.IsInvalid ())
				{
					// Auto file type detection or the user
					// canceled the file type prompt
					fileType = TextFile (); // revisit: detect file type
				}
			}
			(*iter)->SetTargetType (fileType);
		}
	}
}

void Selection::SetType (FileType type)
{
	for (ItemVector::Iterator iter = _items.begin ();
		 iter != _items.end ();
		 ++iter)
	{
		(*iter)->SetTargetType (type);
	}
}

void Selection::LinkItems ()
{
	unsigned int itemCount = _items.size ();
	_sortVector.resize (itemCount);
	// Set default sequencing order
	for (unsigned int i = 0; i < itemCount; ++i)
	{
		_sortVector [i] = _items [i];
	}
	// Link children to parents present in the selection
	for (unsigned int i = 0; i < itemCount; ++i)
	{
		Item * item = _items [i];
		if (item->HasEffectiveTarget ())
		{
			UniqueName const & targetUname = item->GetEffectiveTarget ();
			if (targetUname.IsNormalized ())
			{
				Item const * folderItem = _items.IsFolderIncluded (targetUname.GetParentId ());
				item->SetTargetParentItem (folderItem);
			}
		}
		if (item->HasEffectiveSource ())
		{
			UniqueName const & sourceUname = item->GetEffectiveSource ();
			Item const * folderItem = _items.IsFolderIncluded (sourceUname.GetParentId ());
			item->SetSourceParentItem (folderItem);
		}
	}
}

void Selection::AddResolveItem (FileData const * fd)
{
	std::unique_ptr<Item> item (new ExistingItem (fd, Resolve));	// Resolve name conflict
	// Link item to its parent
	Item const * folderItem = _items.IsFolderIncluded (fd->GetUniqueName ().GetParentId ());
	item->SetTargetParentItem (folderItem);
	if (fd->IsRenamedIn (Area::Original))
	{
		UniqueName const & previousName = fd->GetUnameIn (Area::Original);
		folderItem = _items.IsFolderIncluded (previousName.GetParentId ());
		item->SetSourceParentItem (folderItem);
	}
	_items.push_back (std::move(item));
	_sortVector.push_back (_items.back ());
}

void Selection::AddFolderContents (Item const * folderItem,
								   DataBase const & dataBase,
								   bool recursive)
{
	Assert (folderItem != 0);
	Assert (folderItem->IsFolder ());
	GlobalId gid = folderItem->GetItemGid ();
	Assert (gid != gidInvalid);
	Operation operation = folderItem->GetOperation ();
	Assert (operation != Undefined);
	GidList contents;
	dataBase.ListProjectFiles (gid, contents);
	for (GidList::const_iterator iter = contents.begin (); iter != contents.end (); ++iter)
	{
		if (_items.IsIncluded (*iter))
			continue;

		std::unique_ptr<Item> item;
		FileData const * fd = dataBase.GetFileDataByGid (*iter);
		if (operation == Workspace::Undo && fd->GetState ().IsRelevantIn (Area::Original))
		{
			// Operation on the folder will be undone and it contains checked-out files/folders.
			// If the checked-out file/folders are not included in the original undo selection
			// then here we can only uncheck them out.
			item.reset (new ExistingItem (fd, Workspace::Uncheckout));
		}
		else
		{
			// Perform the same operation as on the folder on the contents 
			item.reset (new ExistingItem (fd, operation));
		}
		if (fd->GetType ().IsFolder ())
		{
			if (recursive)
			{
				// If recursive add folder and its contents,
				// otherwise skip the folder
				_items.push_back (std::move(item));
				AddFolderContents (_items.back (), dataBase);
			}

		}
		else
		{
			_items.push_back (std::move(item));
		}
	}
}

// folderGid is a parent of an item that has effective target (not being deleted)
// Returns pointer to the first added folder or 0
// Add folders bottom-up
Item const * Selection::AddFoldersIfNecessary (GlobalId folderGid, DataBase const & dataBase)
{
	Assert (folderGid != gidInvalid);
	FileData const * fd = dataBase.GetFileDataByGid (folderGid);
	if (fd->GetType ().IsRoot ())
		return 0;	// Folder is project root

	// Add to the selection every folder on the path
	// that is deleted by user or by synch
	FileState state = fd->GetState ();
	if (state.IsNone () && !_items.IsFolderIncluded (folderGid))
	{
		// We are trying to add folder that is no longer under Code Co-op control.
		// The user has to select that folder in the original selection.
		GlobalIdPack pack (folderGid);
		throw Win::Exception (_incompleteOriginalSelection, pack.ToString ().c_str ());
	}

	Item * prevItem = 0;
	Item const * firstAddedFolderItem = 0;
	while (state.IsToBeDeleted () || state.IsSynchDelete ())
	{
		Item const * curItem = _items.IsFolderIncluded (folderGid);
		if (curItem == 0)
		{
			// Required folder not present in the selection -- add it
			std::unique_ptr<Item> item (new ExistingItem (fd, state.IsToBeDeleted () ? Uncheckout : Checkout));
			_items.push_back (std::move(item));
			curItem = _items.back ();
			if (prevItem == 0) // first loop iteration
			{
				Assert (firstAddedFolderItem == 0);
				firstAddedFolderItem = curItem;
			}
			else
			{
				// Link previous item to the added parent
				prevItem->SetTargetParentItem (curItem);
			}
			_sortVector.push_back (curItem);
			prevItem = _items.back ();
		}
		else
		{
			// Folder already in the selection
			if (prevItem != 0 && prevItem->GetTargetParentItem () == 0)
				prevItem->SetTargetParentItem (curItem);
			Assert (prevItem == 0 // first iteration
				|| prevItem == _items.back () // we have added it in the previous iteration
				|| prevItem->GetTargetParentItem () == curItem); // it's already linked in
			prevItem = const_cast<Item *>(curItem);
		}
		fd = dataBase.GetParentFileData (fd);
		state = fd->GetState ();
		folderGid = fd->GetGlobalId ();
	}
	return firstAddedFolderItem;
}

// Returns pointer to the last added folder or 0
// Adds folders top-down
Item const * Selection::ParseUname (UniqueName const & fileUname,
									std::map<UniqueName, Item const *> & nonProjectFolders)
{
	Assert (!fileUname.IsNormalized ());
	// For every relative path segment add item to the extended selection
	// Unique name -- <anchor gid>\FolderA\FolderB\FileName
	// adds the following items:
	//    n    : target uname = <anchor gid>\FolderA, operation = Add, target folder index = 0
	//    n + 1: target uname = <anchor gid>\FolderA\FolderB, operation = Add, target folder index = n
	// Method will return index n + 1.
	// Notice -- we don't add item representing <FileName>, only path segment items are added
	GlobalId anchorGid = fileUname.GetParentId ();
	Assert (anchorGid != gidInvalid);
	FilePath workPath;
	// Check if anchor gid is in the selection
	Item const * currentParentItem = _items.IsFolderIncluded (anchorGid);
	for (PartialPathSeq pathSeq (fileUname.GetName ().c_str ()); !pathSeq.IsLastSegment (); pathSeq.Advance ())
	{
		workPath.DirDown (pathSeq.GetSegment ());
		UniqueName currentUname (anchorGid, workPath.GetDir ());
		std::map<UniqueName, Item const *>::const_iterator uname = nonProjectFolders.find (currentUname);
		if (uname == nonProjectFolders.end ())
		{
			// Unique name seen for the first time -- add item to the extended selection
			std::unique_ptr<Item> item (new ToBeAddedItem (currentUname, currentParentItem));
			_items.push_back (std::move(item));
			currentParentItem = _items.back ();
			_sortVector.push_back (currentParentItem);
			// For every relative path segment we add unique name the nonProjectFolders map
			// Unique name -- <anchor gid>\FolderA\FolderB\FileName
			// Produces the following unames:
			//    <anchor gid>\FolderA --> n
			//    <anchor gid>\FolderA\FolderB --> n + 1
			// Every uname maps to the index to extended selection item
			nonProjectFolders [currentUname] = currentParentItem;
		}
		else
		{
			// Unique name already seen -- remember its item index in the extended selection
			currentParentItem = uname->second;
		}
	}
	return currentParentItem;
}

//////////////////////////////////////////////////////////////////////
//
// Sub-Selection
//

SubSelection::SubSelection (Selection const & selection, GidSet const & gidSubset)
{
	for (Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		Item const * item = seq.GetItemPtr ();
		GlobalId gid = item->GetItemGid ();
		if (gidSubset.find (gid) != gidSubset.end ())
		{
			// Item included in the sub-selection
			_sortVector.push_back (item);
		}
	}
	Assert (_items.size () == 0);	// Sub-selection doesn't have items of its own
									// It always uses items of some other selection
}

//////////////////////////////////////////////////////////////////////
//
// Check-in Selection
//

CheckinSelection::CheckinSelection (GidList const & files, DataBase const & dataBase)
	: Selection (files, dataBase, Checkin)
{}

void CheckinSelection::Extend (DataBase const & dataBase)
{
	unsigned int originalItemCount = _items.size ();
	// We don't use iterator because items are added to the _items vector
	// and we don't want to iterate over added items
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item * curItem = _items [i];
		Assert (curItem->GetOperation () == Checkin);
		if (!curItem->IsTargetParentInSelection () && curItem->HasEffectiveTarget ())
		{
			// Item's parent was not in the original selection.
			// (we know that because selected items are always linked by LinkItems ())
			// Should we add it?
			UniqueName const & itemUname = curItem->GetEffectiveTarget ();
			Assume (itemUname.IsNormalized (), itemUname.GetName ().c_str ());
			FileData const * curParent = dataBase.GetFileDataByGid (itemUname.GetParentId ());
			FileState curParentState = curParent->GetState ();
			Item * curChild = curItem;
			while (curParentState.IsNew ())
			{
				Item const * parentItem = _items.IsFolderIncluded (curParent->GetGlobalId ());
				if (parentItem != 0)
				{
					// Parent folder IS in the selection
					// This is only possible if the parent (and its parents)
					// have been added during Extend.
					Assert (!curChild->IsTargetParentInSelection ());
					// Link current child to the parent
					curChild->SetTargetParentItem (parentItem);
					break;
				}

				// Required folder not present in the selection -- add it
				std::unique_ptr<Item> item (new ExistingItem (curParent, Checkin));
				_items.push_back (std::move(item));
				Item * newParentItem = _items.back ();
				// Link current child to the added parent
				curChild->SetTargetParentItem (newParentItem);
				_sortVector.push_back (newParentItem);
				// Added folder becomes our current child
				curChild = newParentItem;
				curParent = dataBase.GetParentFileData (curParent);
				curParentState = curParent->GetState ();
			}
		}

		if (curItem->IsFolder ())
			AddCheckedOutFolderContents (curItem, dataBase);
	}
	Assert (_items.size () == _sortVector.size ());
}

void CheckinSelection::XMerge (XGroup const & workgroup)
{
	Assert (_items.size () != 0);
	// We don't add items to the check-in selection during merge.
	// If something is missing we abort the check-in.
	for (unsigned int i = 0; i < _items.size (); ++i)
	{
		Item const * curItem = _items [i];
		if (curItem->GetOperation () == Uncheckout)
			continue;	// No changes detected in the check-in selection, instead of check-in
						// we are performing uncheck-out, so we don't have to merge selection with
						// the workgroup.

		Assert (curItem->GetOperation () == Checkin);
		if (!curItem->HasEffectiveTarget ())
			continue;

		UniqueName const & targetUname = curItem->GetEffectiveTarget ();
		Assert (workgroup.IsUsing (targetUname) == 0 ||
				workgroup.IsUsing (targetUname)->GetState ().IsNew ());
		FileData const * conflict = workgroup.WasUsing (targetUname);
		if (conflict != 0)
		{
			// Workgroup item was using selection item's target name
			Incomplete (conflict, "file name conflict");
			throw Win::Exception ();
		}
		if (curItem->IsFolder () && !curItem->HasEffectiveTarget ())
		{
			conflict = workgroup.IsFolderContentsPresent (curItem->GetItemGid ());
			if (conflict != 0)
			{
				// Deleted folder contents present in the workgroup
				Incomplete (conflict, "creation of orphaned file");
				throw Win::Exception ();
			}
		}
	}
	Assert (_items.size () == _sortVector.size ());
}

void CheckinSelection::AddCheckedOutFolderContents (Item const * folderItem,
													DataBase const & dataBase)
{
	Assert (folderItem != 0);
	Assert (folderItem->IsFolder ());
	GlobalId gidFolder = folderItem->GetItemGid ();
	Assert (gidFolder != gidInvalid);
	Operation operation = folderItem->GetOperation ();
	Assert (operation == Checkin);
	GidList checkedOutFiles;
	dataBase.ListCheckedOutFiles (checkedOutFiles);
	for (GidList::const_iterator iter = checkedOutFiles.begin (); iter != checkedOutFiles.end (); ++iter)
	{
		GlobalId gid = *iter;
		if (IsIncluded (gid))
			continue;

		FileData const * fd = dataBase.GetFileDataByGid (gid);
		if (fd->GetUniqueName ().GetParentId () == gidFolder)
		{
			std::unique_ptr<Item> item (new ExistingItem (fd, operation));
			item->SetTargetParentItem (folderItem);
			item->SetSourceParentItem (folderItem);
			_items.push_back (std::move(item));
			_sortVector.push_back (_items.back ());
			if (fd->GetType ().IsFolder ())
				AddCheckedOutFolderContents (_items.back (), dataBase);
		}
	}
}

void CheckinSelection::KeepCheckedOut ()
{
	for (ItemVector::Iterator iter = _items.begin ();
		 iter != _items.end ();
		 ++iter)
	{
		(*iter)->SetKeepCheckedOut (true);
	}
}

class IsChanged : public std::unary_function<Item const *, bool>
{
public:
	IsChanged (PathFinder & pathFinder)
		: _pathFinder (pathFinder)
	{}

	bool operator () (Item const * item) const
	{
		return item->IsChanged (_pathFinder);
	}

private:
	PathFinder &	_pathFinder;
};

bool CheckinSelection::DetectChanges (PathFinder & pathFinder) const
{
	ItemVector::Iterator iter = std::find_if (_items.begin (), _items.end (), IsChanged (pathFinder));
	return iter != _items.end ();
}

// Parent is in project and, if it's New, is in the selection
class IsParentOk : public std::unary_function<Item const *, bool>
{
public:
	IsParentOk (DataBase const & dataBase)
		: _dataBase (dataBase)
	{}

	bool operator () (Item const * item) const
	{
		if (!item->IsTargetParentInSelection () && item->HasEffectiveTarget ())
		{
			// Item's parent is not included in the selection
			UniqueName const & itemUname = item->GetEffectiveTarget ();
			FileData const * futureParent = _dataBase.FindByGid (itemUname.GetParentId ());
			if (futureParent != 0)
			{
				FileState parentState = futureParent->GetState ();
				return parentState.IsPresentIn (Area::Project) && !parentState.IsNew ();
			}
			return false;
		}
		return true;
	}

private:
	DataBase const &	_dataBase;
};

bool CheckinSelection::IsComplete (DataBase const & dataBase) const
{
	ItemVector::Iterator iter = std::find_if (_items.begin (), _items.end (), 
												std::not1 (IsParentOk (dataBase)));
	if (iter != _items.end ())
	{
		std::string info ("Cannot check-in the following ");
		Item const * item = *iter;
		Assert (!item->IsTargetParentInSelection () && item->HasEffectiveTarget ());
		if (item->IsFolder ())
			info += "folder";
		else
			info += "file";
		info += ":\n\n";
		UniqueName const & itemUname = item->GetEffectiveTarget ();
		FileData const * parent = dataBase.FindByGid (itemUname.GetParentId ());
		if (parent != 0)
		{
			Project::Path projectPath (dataBase);
			info += projectPath.MakePath (itemUname);
		}
		else
		{
			info += itemUname.GetName ();
		}
		info += "\n\nbecause its parent is not in the project.";
		TheOutput.Display (info.c_str ());
		return false;
	}
	return true;
}

void CheckinSelection::Incomplete (FileData const * missing, char const * problem)
{
	std::string info ("In order to avoid ");
	info += problem;
	info += ",\nyou have to also check-in the following ";
	if (missing->GetType ().IsFolder ())
		info += "folder";
	else
		info += "file";
	info += ":\n\n";
	info += missing->GetUniqueName ().GetName ();
	GlobalIdPack pack (missing->GetGlobalId ());
	info += ' ';
	info += pack.ToBracketedString ();
	TheOutput.Display (info.c_str ());
}

//////////////////////////////////////////////////////////////////////
//
// Script Selection
//

// Used during unpack file script
ScriptSelection::ScriptSelection (CommandList const & cmdList)
{
	for (CommandList::Sequencer seq (cmdList); !seq.AtEnd (); seq.Advance ())
	{
		FileCmd const & cmd = seq.GetFileCmd ();
		std::unique_ptr<Item> item (new ScriptItem (cmd));
		_items.push_back (std::move(item));
	}
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

// Extend script selection before script execution
// Proof of correctness:
// Outer loop:
//    For each item in original selection, check if the item's target folder is in selection
//    If it's not (and the item itself is not being deleted)
//       Check if the folder is locally deleted. If so, 
//          add it (and its parents, recursively) to the selection with the action "uncheckout"
// 1. First turn of the inner loop:
// Preconditions: x is the item, P is its parent
//   - x is not being deleted. 
//   - P is locally deleted, but was not in the original selection 
//     (hasn't been linked to its target folder)
// Two possibilities:
//   - P is in the selection: we link x to P. 
//     P must have been added in a previous turn of the outer loop, hence its parent has been recursively checked. 
//     therefore no need to continue recursion
//   - P is not in the selection: we add P to selection, link x to P and continue looping
// 2. Second and further turns of the loop
// Preconditions: P is the item that has been just added, Q is its parent.
//   - Q is locally deleted (but this time we cannot assume that it hasn't been in the original selection!)
// Two possibilities:
//   - Q is in the selection: we link P to Q. If Q hasn't been in the original selection, see previous argument
//     If Q has been in the original selection, than it has either been processed or will be processed
//     in the outer loop, so we don't have to continue recursing to its parent.
//   - Q is not in the selection: we add Q, link P to Q and continue looping.
       
void ScriptSelection::Extend (DataBase const & dataBase)
{
	unsigned int originalItemCount = _items.size ();
	// We don't use iterator because items are added to the _items vector
	// and we don't want to iterate over added items
	for (unsigned int i = 0; i < originalItemCount; ++i)
	{
		Item * curItem = _items [i];
		if (!curItem->IsTargetParentInSelection () && curItem->HasEffectiveTarget ())
		{
			// Item's target folder is not in the selection. Should we add it?
			UniqueName const & targetUname = curItem->GetEffectiveTarget ();
			FileData const * targetFolder = dataBase.GetFileDataByGid (targetUname.GetParentId ());
			FileState targetFolderState = targetFolder->GetState ();
			Item * curChild = curItem;
			// Uncheckout target if it is locally deleted -- no script item can
			// end up in the locally deleted folder -- local deletion has to be canceled by uncheckout
			// Note: is target folder is in the state "not in project" (none), it means that the local
			// folder deletion has been checked in and will be restored during script
			// conflict resolution
			while (targetFolderState.IsToBeDeleted () || targetFolderState.IsNone ())
			{
				Item const * folderItem = _items.IsFolderIncluded (targetFolder->GetGlobalId ());
				if (folderItem != 0)
				{
					// Parent present in selection
					Assert (!curChild->IsTargetParentInSelection () // added 
							|| curChild->GetTargetParentItem () == folderItem); // original selection
					// Link current child to the parent
					curChild->SetTargetParentItem (folderItem);
					// Parent was in selection:
					// - Has been added by the outer loop, which also took care of its parent. No need to loop.
					// - Has been there originally. No need to loop, since the outer loop takes care of it.
					break;
				}
				
				// Parent folder not present in the selection -- add it
				std::unique_ptr<Item> item (new ExistingItem (targetFolder, Uncheckout));
				_items.push_back (std::move(item));
				_sortVector.push_back (_items.back ());
				// Link current item to the added parent
				curChild->SetTargetParentItem (_items.back ());
				// Continue looping
				curChild = _items.back ();
				targetFolder = dataBase.GetParentFileData (targetFolder);
				targetFolderState = targetFolder->GetState ();
			}
		}
	}
	Assert (_items.size () == _sortVector.size ());
}

void ScriptSelection::MarkRestored (HistorySelection const & conflict)
{
	for (unsigned int i = 0; i < _items.size (); ++i)
	{
		Workspace::ExistingItem * item = _items.GetExistingItem (i);
		Assert (item != 0);
		GlobalId gid = item->GetItemGid ();
		item->SetRestored (conflict.IsIncluded (gid));
	}
}

//////////////////////////////////////////////////////////////////////
//
// History Selection
//

void HistorySelection::Extend (DataBase const & dataBase)
{
	try
	{
		Selection::Extend (dataBase);
	}
	catch (Win::Exception & ex)
	{
		if (strcmp (ex.GetMessage (), Selection::_incompleteOriginalSelection) == 0)
		{
			// We cannot extend original selection because it is incomplete
			Project::Path projPath (dataBase);
			GlobalIdPack pack (ex.GetObjectName ());
			std::string info ("In order to avoid creation of orphan files/folders in the project,\n"
							  "you also have to revert the following folder:\n\n");
			info += projPath.MakePath (pack);
			info += ' ';
			info += pack.ToBracketedString ();
			TheOutput.Display (info.c_str ());
			throw Win::Exception ();
		}
		throw ex;
	}
}

void HistorySelection::AddCommand (std::unique_ptr<FileCmd> cmd)
{
	GlobalId gid = cmd->GetGlobalId ();
	Assert (gid != gidInvalid);
	GidIterator iter = _gid2Idx.find (gid);
	if (iter != _gid2Idx.end ())
	{
		// Item representing changed file/folder aleady present in the selection
		HistoryItem * item = iter->second;
		item->AddCmd (std::move(cmd));
	}
	else
	{
		// File/folder seen for the first time
		std::unique_ptr<Item> item (new HistoryItem (std::move(cmd)));
		_items.push_back (std::move(item));
		_gid2Idx [gid] = _items.GetHistoryItem (_items.size () - 1);
	}
}

void HistorySelection::Erase(GidSet const & eraseSet)
{
	_items.Erase(eraseSet);
}

bool HistorySelection::IsRecoverable (GlobalId gid) const
{
	GidIterator iter = _gid2Idx.find (gid);
	if (iter != _gid2Idx.end ())
	{
		HistoryItem const * item = iter->second;
		return item->IsRecoverable ();
	}
	return true;
}

void HistorySelection::Init (bool isForward)
{
	Assert (_sortVector.empty ());
	Operation operation = isForward ? Redo : Undo;
	SetOperation (operation);
	LinkItems ();
	Assert (_items.size () == _sortVector.size ());
}

HistoryRangeSelection::HistoryRangeSelection (History::Db const & history,
											  History::Range const & range,
											  bool isForward,
											  Progress::Meter & meter)
{
	Assert (range.GetYoungestId () != gidInvalid);
	Assert (range.GetOldestId () != gidInvalid);
	History::RangeAllFiles filter (*this,
								   range.GetYoungestId (),
								   history.GetPredecessorId (range.GetOldestId ()));
	history.RetrieveSetCommands (filter, meter);
	Init (isForward);
}

FilteredHistoryRangeSelection::FilteredHistoryRangeSelection (History::Db const & history,
															  History::Range const & range,
															  GidSet const & preSelectedGids,
															  bool isForward,
															  Progress::Meter & meter)
{
	Assert (range.GetYoungestId () != gidInvalid);
	Assert (range.GetOldestId () != gidInvalid);
	Assert (!preSelectedGids.empty ());

	History::RangeSomeFiles filter (*this,
									range.GetYoungestId (),
									history.GetPredecessorId (range.GetOldestId ()),
									preSelectedGids);
	history.RetrieveSetCommands (filter, meter);
	Init (isForward);
}

RepairHistorySelection::RepairHistorySelection (History::Db const & history,
												VerificationReport::Sequencer corruptedFiles,
												GidSet & unrecoverableFiles,
												Progress::Meter & meter)
{
	History::RepairFilter filter (*this, corruptedFiles, history.MostRecentScriptId ());
	history.RetrieveSetCommands (filter, meter);
	filter.MoveUnrecoverableFiles (unrecoverableFiles);
	Init (true);
}

BlameHistorySelection::BlameHistorySelection (History::Db const & history,
											  GlobalId fileGid,
											  Progress::Meter & meter)
{
	History::BlameFilter filter (*this, fileGid, _scriptIds, history.MostRecentScriptId ());
	history.RetrieveSetCommands (filter, meter);
	Init (true);	// Forward selection
}

/////////////////////////////////////////////////////////////////////////
//
// Topological Sorter
//

TopologicalSorter::TopologicalSorter (Selection::ItemVector const & selection)
{
	// Prepare for topological sort -- go over selection items and create sort items
	std::map<Item const *, unsigned int> selectionItem2sortItemIdx;
	typedef std::map<Item const *, unsigned int>::const_iterator Iterator;
	for (Selection::ItemVector::Iterator iter = selection.begin (); iter != selection.end (); ++iter)
	{
		Workspace::Item const * selectionItem = *iter;
		SortItem sortItem (selectionItem);
		_sortItems.push_back (sortItem);
		selectionItem2sortItemIdx [selectionItem] = _sortItems.size () - 1;
	}

	// Establish predecessor-successor relation.
	// For every selection item:
	//     if item's effective target folder is in the selection
	//	       increase item predecessor count
	//         add item as effective target folder sucessor
	//
	//     if item's effective source folder is in the selection
	//         increase source folder predecessor count
	//         add effective source folder as item sucessor
	for (unsigned int itemIdx = 0; itemIdx < selection.size (); ++itemIdx)
	{
		Item const * selectionItem = selection [itemIdx];
		Item const * targetFolderItem = selectionItem->GetTargetParentItem ();
		SortItem & curSortItem = _sortItems [itemIdx];
		if (targetFolderItem != 0)
		{
			// Effective target folder is in the selection
			Iterator iter = selectionItem2sortItemIdx.find (targetFolderItem);
			Assert (iter != selectionItem2sortItemIdx.end ());
			unsigned int idx = iter->second;
			Assert (idx < _sortItems.size ());
			SortItem & targetFolderSortItem = _sortItems [idx];
			Assume (targetFolderItem->GetOperation () != Delete, "File move to deleted folder");
			// Increase item predecessor count
			curSortItem.IncPredecessorCount ();
			// Add item as effective target folder sucessor
			targetFolderSortItem.AddSuccessor (&curSortItem);
		}

		Item const * sourceFolderItem = selectionItem->GetSourceParentItem ();
		// Cannot delete a source folder before its content is moved or deleted
		if (sourceFolderItem != 0 && sourceFolderItem != targetFolderItem && sourceFolderItem->HasBeenDeleted())
		{
			// Effective source folder is in the selection
			Iterator iter = selectionItem2sortItemIdx.find (sourceFolderItem);
			Assert (iter != selectionItem2sortItemIdx.end ());
			unsigned int idx = iter->second;
			Assert (idx < _sortItems.size ());
			SortItem & sourceFolderSortItem = _sortItems [idx];
			Assume (sourceFolderItem->GetOperation () != Add, "File move from new folder");
			// Increase source folder predecessor count
			sourceFolderSortItem.IncPredecessorCount ();
			// Add effective source folder as item sucessor
			curSortItem.AddSuccessor (&sourceFolderSortItem);
		}
	}
	Assert (_sortItems.size () == selection.size ());
}

void TopologicalSorter::Sort (std::vector<Item const *> & sortVector)
{
	Assert (_sortItems.size () == sortVector.size ());
	std::vector<SortItem *> zeroes;
	// Create a vector of sort items pointers, to sort items without predecessors
	for (std::vector<SortItem>::iterator iter = _sortItems.begin (); iter != _sortItems.end (); ++iter)
	{
		SortItem & sortItem = *iter; 
		if (sortItem.HasNoPredecessors ())
			zeroes.push_back (&sortItem);
	}
	Assert (!zeroes.empty ());
	unsigned int sortVectorIdx = 0;
	while (!zeroes.empty ())
	{
		SortItem const * sortItem = zeroes.back ();
		sortVector [sortVectorIdx] = sortItem->GetSelectionItem ();
		sortVectorIdx++;
		zeroes.pop_back ();
		// Go through list of successors and decrement their respective
		// predecessors count. Items without precessors are added to the zeroes vector.
		std::vector<SortItem *> const & successors = sortItem->GetSucessors ();
		for (std::vector<SortItem *>::const_iterator iter = successors.begin ();
			 iter != successors.end ();
			 iter++)
		{
			SortItem * item = *iter;
			item->DecPredecessorCount ();
			if (item->HasNoPredecessors ())
				zeroes.push_back (item);
		}
	}
	Assert (sortVectorIdx == sortVector.size ());
}

#if !defined (NDEBUG) && !defined (BETA)
// Unit Test
void VerifyTestSort(std::vector<Workspace::Item const *> const & sortVector)
{
	std::vector<Workspace::Item const *>::const_iterator it = sortVector.begin();
	while(it != sortVector.end())
	{
		Workspace::Item const * item = *it;
		Workspace::Item const * itemTarget= item->GetTargetParentItem ();
		Workspace::Item const * itemSource= item->GetSourceParentItem ();

		std::vector<Workspace::Item const *>::const_iterator it2 = it;
		++it2;
		while (it2 != sortVector.end())
		{
			Workspace::Item const * lesserItem = *it2;
			Workspace::Item const * lesserTarget= lesserItem->GetTargetParentItem ();
			Workspace::Item const * lesserSource= lesserItem->GetSourceParentItem ();
            Assert(!(itemTarget != nullptr && itemTarget == lesserItem));
            Assert(!(lesserSource != nullptr && item->HasBeenDeleted() && lesserSource == item));
			++it2;
		}
		++it;
	}
}

void PrepareTestItems(Selection::ItemVector & items, int i)
{
	if (i == 0)
	{
		// move file down one level
		std::unique_ptr<Workspace::Item> file(new Workspace::TestItem(1));
		std::unique_ptr<Workspace::Item> srcFolder(new Workspace::TestItem(2, true));
		std::unique_ptr<Workspace::Item> tgtFolder(new Workspace::TestItem(3, true));
		file->SetSourceParentItem(srcFolder.get());
		file->SetTargetParentItem(tgtFolder.get());
		tgtFolder->SetSourceParentItem(srcFolder.get());
		tgtFolder->SetTargetParentItem(srcFolder.get());
		items.push_back(std::move(file));
		items.push_back(std::move(srcFolder));
		items.push_back(std::move(tgtFolder));
	}
	else
	{
		// move two files up two levels and delete source and middle folder
		std::unique_ptr<Workspace::Item> file1(new Workspace::TestItem(1));
		std::unique_ptr<Workspace::Item> file2(new Workspace::TestItem(2));
		std::unique_ptr<Workspace::Item> srcFolder(new Workspace::TestItem(3, true, true));
		std::unique_ptr<Workspace::Item> middleFolder(new Workspace::TestItem(4, true, true));
		std::unique_ptr<Workspace::Item> tgtFolder(new Workspace::TestItem(5, true));
		file1->SetSourceParentItem(srcFolder.get());
		file1->SetTargetParentItem(tgtFolder.get());
		file2->SetSourceParentItem(srcFolder.get());
		file2->SetTargetParentItem(tgtFolder.get());
		srcFolder->SetSourceParentItem(middleFolder.get());
		middleFolder->SetSourceParentItem(tgtFolder.get());
		items.push_back(std::move(middleFolder));
		items.push_back(std::move(file1));
		items.push_back(std::move(srcFolder));
		items.push_back(std::move(tgtFolder));
		items.push_back(std::move(file2));
	}
}

void TopoSortTest ()
{
	for (int i = 0; i < 2; ++i)
	{
		Selection::ItemVector items;
		PrepareTestItems(items, i);
		TopologicalSorter sorter(items);
		std::vector<Workspace::Item const *> sortVector;
		sortVector.resize(items.size());
		sorter.Sort(sortVector);
		VerifyTestSort(sortVector);
		items.clear();
		sortVector.clear();
	}
}

#endif
