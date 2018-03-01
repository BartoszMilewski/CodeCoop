//------------------------------------
//	(c) Reliable Software, 1997 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "SynchArea.h"
#include "ScriptCommands.h"
#include "DataBase.h"
#include "History.h"
#include "PathFind.h"
#include "PhysicalFile.h"
#include "OutputSink.h"

#include <Dbg/Assert.h>
#include <Ex/WinEx.h>

char const * SynchArea::_kindName [] =
{
	"Added",
	"Edited",
	"Deleted",
	"Renamed",
	"None",
	"Restored",
	"Moved",
	"Removed"
};

// Script File Name

void SynchScriptInfo::Serialize (Serializer& out) const
{
}

void SynchScriptInfo::Deserialize (Deserializer& in, int version)
{
	_scriptId = in.GetLong ();
	_refCount = in.GetLong ();
	_name.Deserialize (in, version);
}

// Synch File Information

SynchItem::SynchItem (FileCmd const & fileCmd)
	: _parentGid (fileCmd.GetUniqueName ().GetParentId ()),
	  _fileGid (fileCmd.GetGlobalId ()),
	  _synchKind (fileCmd.GetSynchKind ()),
	  _name (fileCmd.GetUniqueName ().GetName ()),
	  _type (fileCmd.GetFileType ()),
	  _checkSum (fileCmd.GetNewCheckSum ())
{}

SynchItem::SynchItem (FileData const & fileData, CheckSum checkSum)
	: _parentGid (fileData.GetUniqueName ().GetParentId ()),
	  _fileGid (fileData.GetGlobalId ()),
	  _synchKind (synchRestore),
	  _name (fileData.GetUniqueName ().GetName ()),
	  _type (fileData.GetType ()),
	  _checkSum (checkSum)
{}

void SynchItem::Serialize (Serializer& out) const
{
	out.PutLong (_parentGid);
	out.PutLong (_fileGid);
	out.PutLong (_synchKind);
	_name.Serialize (out);
	out.PutLong (_type.GetValue ());
	out.PutLong (_checkSum.GetSum ());
	out.PutLong (_checkSum.GetCrc ());
}

void SynchItem::Deserialize (Deserializer& in, int version)
{
	_parentGid = in.GetLong ();
	_fileGid   = in.GetLong ();
	_synchKind = (SynchKind) in.GetLong ();
	_name.Deserialize (in, version);
	_type.Init (in.GetLong ());
	if (!_type.Verify ())
		throw Win::Exception ("Error reading synch item: invalid file type");
	unsigned long sum = in.GetLong ();
	unsigned long crc = CheckSum::crcWildCard;
	if (version >= 35)
		crc = in.GetLong ();
	_checkSum.Init (sum, crc);
}

// Synch Area

SynchArea::SynchArea (DataBase const & dataBase, PathFinder & pathFinder) 
	: _dataBase (dataBase),
	  _pathFinder (pathFinder)
{
	AddTransactableMember (_scriptId);
	AddTransactableMember (_scriptComment);
	AddTransactableMember (_synchItems);
}

bool SynchArea::IsEmpty (std::string const & curOperation) const
{
	if (IsEmpty ())
		return true;
	std::string msg ("Synch Area is not empty.\nAccept previous synchronization script\nbefore ");
	msg += curOperation;
	TheOutput.Display (msg.c_str ());
	return false;
}

void SynchArea::XAddScriptFile (std::string const & scriptComment, GlobalId scriptId)
{
	_scriptId.XSet (scriptId);
	_scriptComment.XSet (scriptComment);
}

void SynchArea::XAddSynchFile (FileCmd const & fileCmd)
{
	std::unique_ptr<SynchItem> newItem (new SynchItem (fileCmd));
	CheckForDuplicates (*newItem);
	_synchItems.XAppend (std::move(newItem));
	Notify (changeAdd, fileCmd.GetGlobalId ());
}

void SynchArea::XAddSynchFile (FileData const & fileData, CheckSum checkSum)
{
	std::unique_ptr<SynchItem> newItem (new SynchItem (fileData, checkSum));
	CheckForDuplicates (*newItem);
	_synchItems.XAppend (std::move(newItem));
	Notify (changeAdd, fileData.GetGlobalId ());
}

void SynchArea::CheckForDuplicates (SynchItem const & newItem) const
{
	FileData const * fileData = _dataBase.XGetFileDataByGid (newItem.GetGlobalId ());
	if (!fileData->GetState ().IsRelevantIn (Area::Synch))
		throw Win::Exception ("Adding non-relevant file to synch area.\nPlease contact support@relisoft.com", newItem.GetName ().c_str (), 0);
	unsigned count = _synchItems.XCount ();
	for (unsigned i = 0; i < count; ++i)
	{
		SynchItem const * item = _synchItems.XGet (i);
		if (item != 0 && item->GetGlobalId () == newItem.GetGlobalId ())
		{
			throw Win::Exception ("Duplicate item in synch area.\nPlease contact support@relisoft.com", newItem.GetName ().c_str (), 0);
		}
	}
}

CheckSum SynchArea::XGetCheckSum (GlobalId gid) const
{
	SynchItem const * item = XFindSynchItem (gid);
	Assert (item != 0);
	return item->GetCheckSum ();
}

void SynchArea::XUpdateCheckSum (GlobalId gid, CheckSum newChecksum)
{
	SynchItem * item = XFindEditSynchItem (gid);
	Assert (item != 0);
	item->SetCheckSum (newChecksum);
}

void SynchArea::XUpdateName (GlobalId gid, std::string const & newName)
{
	SynchItem * item = XFindEditSynchItem (gid);
	Assert (item != 0);
	item->SetName (newName);
}

void SynchArea::XRemoveSynchFile (GlobalId gid)
{
	XRemoveSynchItem (gid);
	Notify (changeRemove, gid);
	if (_synchItems.XActualCount () == 0)
	{
		// Last synch item removed -- clear script id and comment
		_scriptId.XSet (gidInvalid);
		_scriptComment.XSet (std::string ());
	}
}

bool SynchArea::IsPresent (GlobalId gid) const
{
	return FindSynchItem (gid) != 0;
}

void SynchArea::Verify (PathFinder & pathFinder) const
{
	// Check if all files that should be in the Synch Area
	// really exist there
	for (unsigned int i = 0; i < _synchItems.Count (); i++)
	{
		SynchItem const * synchItem = _synchItems.Get (i);
		if (synchItem->GetType ().IsFolder ())
			continue;
		GlobalId gid = synchItem->GetGlobalId ();
		Assert (gid != gidInvalid);
		FileData const * fileData = _dataBase.GetFileDataByGid (gid);
		Assert (fileData != 0);
		FileState state = fileData->GetState ();
		SynchKind kind = synchItem->GetSynchKind ();
		// Assume (state.IsRelevantIn (Area::Synch)); // Not true after crash
		if (state.IsPresentIn (Area::Synch))
		{
			char const * synchPath = pathFinder.GetFullPath (gid, Area::Synch);
			if (!File::Exists (synchPath))
			{
				std::string info ("Corrupted database; Missing sync copy of file!\n'");
				info += synchPath;
				info += '\'';
				TheOutput.Display (info.c_str (), Out::Error);
			}
		}
		if (state.IsPresentIn (Area::PreSynch))
		{
			char const * preSyncPath = pathFinder.GetFullPath (gid, Area::PreSynch);
			if (!File::Exists (preSyncPath))
			{
				std::string info ("Corrupted database; Missing pre-sync copy of file!\n'");
				info += preSyncPath;
				info += '\''; 
				TheOutput.Display (info.c_str (), Out::Error);
			}
		}
	}
}

void SynchArea::GetFileList (GidList & files) const
{
	for (unsigned int i = 0; i < _synchItems.Count (); i++)
	{
		SynchItem const * synchItem = _synchItems.Get (i);
		files.push_back (synchItem->GetGlobalId ());
	}
}

// Synch Area Transactable Interface

void SynchArea::Serialize (Serializer& out) const
{
	_scriptId.Serialize (out);
	_scriptComment.Serialize (out);
	_synchItems.Serialize (out);
}

void SynchArea::Deserialize (Deserializer& in, int version)
{
	CheckVersion (version, VersionNo ());
	if (version < 36)
	{
		TransactableArray<SynchScriptInfo> scripts;
		scripts.Deserialize (in, version);
		if (scripts.XCount () != 0)
		{
			SynchScriptInfo const * info = scripts.XGet (0);
			_scriptId.XSet (info->GetScriptId ());
			_scriptComment.XSet (info->GetScriptFileName ());
		}
	}
	else
	{
		_scriptId.Deserialize (in, version);
		_scriptComment.Deserialize (in, version);
	}
	_synchItems.Deserialize (in, version);
	if (version < 36)
		XSetParentIds ();
}

void SynchArea::XSetParentIds ()
{
	for (unsigned int i = 0; i < _synchItems.XCount (); ++i)
	{
		SynchItem * item = _synchItems.XGetEdit (i);
		FileData const * fd = _dataBase.XGetFileDataByGid (item->GetGlobalId ());
		item->SetParentId (fd->GetUniqueName ().GetParentId ());
	}
}

// Synch Area Table Interface

void SynchArea::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	if (restrict.HasFiles ())
	{
		// Special restriction pre-filled with unique names
		Restriction::UnameIter it;
		for (it = restrict.BeginFiles (); it != restrict.EndFiles (); ++it)
		{
			UniqueName const & uname = *it;
			FileData const * fd = _dataBase.FindProjectFileByName (uname);
			if (fd != 0)
				ids.push_back (fd->GetGlobalId ());
		}
	}
	else
	{
		GetFileList (ids);
	}
}

bool SynchArea::IsValid () const
{
	return _dataBase.GetMyId () != gidInvalid;
}

std::string SynchArea::GetStringField (Column col, GlobalId gid) const
{
	if (col != colParentPath)
	{
		SynchItem const * synchItem = FindSynchItem (gid);
		Assert (synchItem != 0);
	
		if (col == colName)
		{
			return synchItem->GetName ();
		}
		else if (col == colStateName)
		{
			SynchKind kind = synchItem->GetSynchKind ();
			if (synchNew <= kind && kind < synchLastKind)
				return _kindName [kind];
			else
				return "Unknown";
		}
		Assert (!"SynchArea: Invalid column");
	}
	else
	{
		Assert (col == colParentPath);
		Project::Path parent (_dataBase);
		char const * path = parent.MakePath (gid);
		if (path == 0)
			return std::string ();

		return std::string (path);
	}
	return std::string ();
}

GlobalId SynchArea::GetIdField (Column col, GlobalId gid) const
{
	if (col == colParentId)
	{
		SynchItem const * synchItem = FindSynchItem (gid);
		Assert (synchItem != 0);
		return synchItem->GetParentGid ();
	}
	Assert (!"Asking for id data from the wrong column of SynchArea");
	return gidInvalid;
}

std::string SynchArea::GetStringField (Column col, UniqueName const & uname) const
{
	return std::string ();
}

GlobalId SynchArea::GetIdField (Column col, UniqueName const & uname) const
{
	return gidInvalid;
}

unsigned long SynchArea::GetNumericField (Column col, GlobalId gid) const
{
	if (col == colState)
	{
		FileData const * file = _dataBase.GetFileDataByGid (gid);
		FileState state = file->GetState ();
		FileType type = file->GetType ();
		Assert (!type.IsRoot ());
		Assert (state.IsRelevantIn (Area::Synch));
		if (!type.IsFolder ())
		{
			bool diff = true;
			try
			{
				PhysicalFile physFile (*file, _pathFinder);
				diff = physFile.IsDifferent (Area::Synch, Area::PreSynch);
			}
			catch (...) 
			{
				Win::ClearError ();
			}
			if (diff)
			{
				// Edited
				state.SetCoDiff (diff);
			}
			else if (file->IsRenamedIn (Area::PreSynch))
			{
				UniqueName const & alias = file->GetUnameIn (Area::PreSynch);
				UniqueName const & uname = file->GetUniqueName ();
				if (uname.GetParentId () != alias.GetParentId ())
					state.SetMoved (true);
				else
					state.SetRenamed (true);
			}
		}
		return state.GetValue ();
	}
	else if (col == colType)
	{
		SynchItem const * synchItem = FindSynchItem (gid);
		Assert (synchItem != 0);
		return synchItem->GetType ().GetValue ();
	}
	Assert (!"Invalid numeric column");
	return 0;
}

std::string SynchArea::GetCaption (Restriction const & restrict) const
{
	if (_scriptComment.IsEmpty ())
		return std::string ("No merge conflicts");
	else
		return _scriptComment.c_str ();
}

// Synch Area Private Methods

SynchItem const * SynchArea::XFindSynchItem (GlobalId gid) const
{
	Assert (gidInvalid != gid);
	for (unsigned int i = 0; i < _synchItems.XCount (); i++)
	{
		SynchItem const * synchItem = _synchItems.XGet (i);
		if (synchItem != 0 && gid == synchItem->GetGlobalId ())
		{
			return synchItem;
		}
	}
	return 0;
}

SynchItem * SynchArea::XFindEditSynchItem (GlobalId gid)
{
	Assert (gidInvalid != gid);
	for (unsigned int i = 0; i < _synchItems.XCount (); i++)
	{
		SynchItem * synchItem = _synchItems.XGetEdit (i);
		if (synchItem != 0 && gid == synchItem->GetGlobalId ())
		{
			return synchItem;
		}
	}
	return 0;
}

SynchItem const * SynchArea::FindSynchItem (GlobalId gid) const
{
	Assert (gidInvalid != gid);
	for (unsigned int i = 0; i < _synchItems.Count (); i++)
	{
		SynchItem const * synchItem = _synchItems.Get (i);
		if (gid == synchItem->GetGlobalId ())
		{
			return synchItem;
		}
	}
	return 0;
}

void SynchArea::XRemoveSynchItem (GlobalId gid)
{
	for (unsigned int i = 0; i < _synchItems.XCount (); i++)
	{
		SynchItem const * synchItem = _synchItems.XGet (i);
		if (synchItem != 0 && gid == synchItem->GetGlobalId ())
		{
			_synchItems.XMarkDeleted (i);
		}
	}
	return;
}
