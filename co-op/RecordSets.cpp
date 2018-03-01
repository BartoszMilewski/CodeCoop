//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "RecordSets.h"
#include "Image.h"
#include "OutputSink.h"
#include "SelectIter.h"
#include "ColumnInfo.h"
#include "Global.h"

#include <Dbg/Assert.h>

void GidToStringCache::AddString (GlobalId gid, std::string const & string)
{
	_strings [gid] = string;
}

std::string const & GidToStringCache::GetString (GlobalId gid) const
{
	std::map<GlobalId, std::string>::const_iterator iter = _strings.find (gid);
	if (iter != _strings.end ())
		return iter->second;

	return _unknownString;
}

//
// File Record Set
//

GlobalId FileRecordSet::GetParentId (unsigned int row) const
{
	Assert (_table.IsValid ());
	Assert (row < _ids.size ());
	return _parentIds [row];
}

char const * FileRecordSet::GetParentPath (unsigned int row) const
{
	GlobalId gid = GetParentId (row);
	return _pathCache.GetString (gid).c_str ();
}

void FileRecordSet::UpdatePaths ()
{
	for (GidList::const_iterator iter = _parentIds.begin (); iter != _parentIds.end (); ++iter)
	{
		GlobalId fileGid = *iter;
		if (fileGid == gidInvalid)
			continue;
		std::string parentPath = _table.GetStringField (Table::colParentPath, fileGid);
		_pathCache.AddString (fileGid, parentPath);
	}
}

int FileRecordSet::CmpNames (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());
	if (AreOfSameType (row1, row2))
	{
		// Files or folders
		return RecordSet::CmpNames (row1, row2);
	}
	else
	{
		// Mixed file with folder
		return CmpMixedType (row1, row2);
	}
}

int FileRecordSet::CmpStates (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _states.size ());
	Assert (row1 < _states.size ());
	int result = 0;
	if (AreOfSameType (row1, row2))
	{
		// Files or folders
		FileState row1State = _fileState [row1];
		FileState row2State = _fileState [row2];
		if (row1State.GetValue () != row2State.GetValue ())
		{
			// Different states
			if (!row1State.IsNone () && !row2State.IsNone ())
			{
				// Both controlled
				if (row1State.IsRelevantIn (Area::Original) && row2State.IsRelevantIn (Area::Original))
				{
					// Both checked out
					if (row1State.IsPresentIn (Area::Original) != row2State.IsPresentIn (Area::Original))
					{
						// Different presence in the Area::Original -- new before checked out
						result = row1State.IsPresentIn (Area::Original) ? 1 : -1;
					}
					// else both present or not present in the Area::Original -- compare by name
				}
				else if (row1State.IsRelevantIn (Area::Original) != row2State.IsRelevantIn (Area::Original) ||
						 row1State.IsRelevantIn (Area::Synch)    != row2State.IsRelevantIn (Area::Synch))
				{
					// Different relevance in the Area::Original or Area::Synch
					// Checked out or synced out or restored before checked in
					result = (row1State.IsRelevantIn (Area::Original) ||
							  row1State.IsRelevantIn (Area::Synch)    ||
							  row1State.IsRelevantIn (Area::Reference)) ? -1 : 1;
				}
				// else compare by name
			}
			else
			{
				// One is controlled and the other is not controlled
				// Controlled before non controlled
				result = row1State.IsNone () ? 1 : -1;
			}
		}
		// else compare by name
	}
	else
	{
		// Mixed file with folder
		result = CmpMixedType (row1, row2);
	}

	// States are the same compare file names
	if (result == 0)
		result = RecordSet::CmpNames (row1, row2);

	return result;
}

int FileRecordSet::CmpGids (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _ids.size ());
	Assert (row2 < _ids.size ());
	int result = 0;
	if (AreOfSameType (row1, row2))
	{
		// Files or folders
		result = _ids [row1] - _ids [row2];
	}
	else
	{
		// Mixed file with folder
		result = CmpMixedType (row1, row2);
	}
	return result;
}

int FileRecordSet::CmpTypes (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _types.size ());
	Assert (row2 < _types.size ());

	int result = 0;
	if (_ids [row1] == gidInvalid || _ids [row2] == gidInvalid)
	{		
		if (_ids [row1] != gidInvalid)
			result = -1;
		else if (_ids [row2] != gidInvalid)
			result =  1;
	}

	if (result == 0)
	{
		if (AreOfSameType (row1, row2))
		{
			// Files or folders
			// Compare type names
			result = NocaseCompare (_types [row1].GetName (), _types [row2].GetName ());
			if (result == 0)
			{
				// Type names are identical -- compare file name extensions
				PathSplitter splitter1 (_names [row1]);
				PathSplitter splitter2 (_names [row2]);
				result = NocaseCompare (splitter1.GetExtension (), splitter2.GetExtension ());
			}
		}
		else
		{
			// Mixed file with folder
			result = CmpMixedType (row1, row2);
		}
	}

	// Types are the same compare file names
	if (result == 0)
		result = RecordSet::CmpNames (row1, row2);
	return result;
}

int FileRecordSet::CmpPaths (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _types.size ());
	Assert (row2 < _types.size ());
	if (AreOfSameType (row1, row2))
	{
		// Files or folders
		FilePath row1Path (GetParentPath (row1));
		FilePath row2Path (GetParentPath (row2));
		if (row1Path.IsEqualDir (row2Path))
			return 0;
		else
			return row1Path.IsDirLess (row2Path) ? -1 : 1;
	}

	// Mixed file with folder
	return CmpMixedType (row1, row2);
}

int FileRecordSet::CmpChanged (unsigned int row1, unsigned int row2) const
{
	Assert (_table.IsValid ());
	Assert (row1 < _fileState.size ());
	Assert (row2 < _fileState.size ());
	int result = 0;
	FileState state1 = _fileState [row1];
	FileState state2 = _fileState [row2];

	bool const is1InProject = state1.IsPresentIn (Area::Project);
	bool const is2InProject = state2.IsPresentIn (Area::Project);
	bool const is1New = state1.IsNew ();
	bool const is2New = state2.IsNew ();

	FileType row1Type = _types [row1];

	if (AreOfSameType (row1, row2))
	{
		if (row1Type.IsFolder ())
		{
			// Two folders:
			//	- new is greater then deleted and checked out
			//  - deleted is greater then checked out
			bool const is1NewOrDeleted = is1New || !is1InProject;
			bool const is2NewOrDeleted = is2New || !is2InProject;
			if (is1NewOrDeleted && !is2NewOrDeleted)
				result = -1;
			else if (!is1NewOrDeleted && is2NewOrDeleted)
				result = 1;
		}
		else
		{
			// Two files
			bool state1ChangesDetected = state1.IsCoDiff ()  ||
										 !is1InProject       ||
										 is1New				 ||
										 state1.IsRenamed () ||
										 state1.IsMoved ()   ||
										 state1.IsMerge ()   ||
										 state1.IsTypeChanged ();
			bool state2ChangesDetected = state2.IsCoDiff ()  ||
										 !is2InProject       ||
										 is2New              ||
										 state2.IsRenamed () ||
										 state2.IsMoved ()   ||
										 state2.IsMerge ()   ||
										 state2.IsTypeChanged ();
			if (state1ChangesDetected && !state2ChangesDetected)
				result = -1;
			else if (!state1ChangesDetected && state2ChangesDetected)
				result = 1;
			else if (state1ChangesDetected && state2ChangesDetected)
			{
				if (state1.IsMerge () && !state2.IsMerge ())
					result = -1;
				else if (!state1.IsMerge () && state2.IsMerge ())
					result = 1;
				else if (state1.IsCoDiff () && !state2.IsCoDiff ())
					result = -1;
				else if (!state1.IsCoDiff () && state2.IsCoDiff ())
					result = 1;
				else if (is1InProject && !is2InProject)
					result = 1;
				else if (!is1InProject && is2InProject)
					result = -1;
				else if (state1.IsMoved () && !state2.IsMoved ())
					result = -1;
				else if (!state1.IsMoved () && state2.IsMoved ())
					result = 1;
				else if (state1.IsRenamed () && !state2.IsRenamed ())
					result = -1;
				else if (!state1.IsRenamed () && state2.IsRenamed ())
					result = 1;
				else if (state1.IsTypeChanged () && !state2.IsTypeChanged ())
					result = -1;
				else if (!state1.IsTypeChanged () && state2.IsTypeChanged ())
					result = 1;
				else if (is1New && !is2New)
					result = -1;
				else if (!is1New && is2New)
					result = 1;
			}
		}
	}
	else
	{
		// File and folder
		result = row1Type.IsFolder () ? 1 : -1;
	}

	// Change bits are the same compare file names
	if (result == 0)
		result = RecordSet::CmpNames (row1, row2);

	return result;
}

bool FileRecordSet::AreOfSameType (unsigned int row1, unsigned int row2) const
{
	FileType row1Type = _types [row1];
	FileType row2Type = _types [row2];
	return (!row1Type.IsFolder () && !row2Type.IsFolder ()) ||
			(row1Type.IsFolder () &&  row2Type.IsFolder ());
}

int FileRecordSet::CmpMixedType (unsigned int row1, unsigned int row2) const
{
	Assert (!AreOfSameType (row1, row2));
	FileType row1Type = _types [row1];
	return row1Type.IsFolder () ? -1 : 1;
}

//
// Script Record Set
//

char const * ScriptRecordSet::GetSenderName (unsigned int row) const
{
	Assert (_table.IsValid ());
	Assert (row < _ids.size ());
	GlobalIdPack scriptId (_ids [row]);
	return _senderCache.GetString (scriptId.GetUserId ()).c_str ();
}

void ScriptRecordSet::UpdateSenders ()
{
	for (GidList::const_iterator iter = _ids.begin (); iter != _ids.end (); ++iter)
	{
		GlobalId scriptId = *iter;
		GlobalId senderId = GlobalIdPack (scriptId).GetUserId ();
		std::string senderName = _table.GetStringField (Table::colFrom, senderId);
		_senderCache.AddString (senderId, senderName);
	
	}
}

void ScriptRecordSet::MultiLine2SingleLine (std::string & buf) const
{
	// Replace all newline characters with space character
	for (std::string::size_type pos = buf.find_first_of ("\r\n");
		 pos != std::string::npos;
		 pos = buf.find_first_of ("\r\n", pos))
	{
		buf.replace (pos, 1, " ");
	}
}

//
// MailBoxRecordSet
//

MailBoxRecordSet::MailBoxRecordSet (Table const & table, Restriction const & restrict)
	: ScriptRecordSet (table)
{
	if (!_table.IsValid ())
		return;

	// Get a listing of script ids
	_table.QueryUniqueIds (restrict, _ids);
	UpdateSenders ();
	int count = _ids.size ();
	// Ror each script id, get the other fields
	for (int i = 0; i < count; i++)
	{
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		_scriptState.push_back (_table.GetNumericField (Table::colState, _ids [i]));
		_timeStamps.push_back (_table.GetStringField (Table::colTimeStamp, _ids [i]));
	}
	InitColumnHeaders (Column::Mailbox);
}

void MailBoxRecordSet::Refresh (unsigned int row)
{
	Assert (_table.IsValid ());
	Assert (_ids [row] != gidInvalid);
	std::string newName (_table.GetStringField (Table::colName, _ids [row]));
	_names [row] = newName;
	std::string newState (_table.GetStringField (Table::colStateName, _ids [row]));
	_states [row] = newState;
	_scriptState [row] = _table.GetNumericField (Table::colState, _ids [row]);
	_timeStamps [row] = _table.GetStringField (Table::colTimeStamp, _ids [row]);
	// Refresh sender cache
	GlobalId senderId = GlobalIdPack (_ids [row]).GetUserId ();
	std::string senderName = _table.GetStringField (Table::colFrom, senderId);
	_senderCache.AddString (senderId, senderName);
}

void MailBoxRecordSet::PushRow (GlobalId gid, std::string const & name)
{
	Assert ("We should never push rows in the mailbox record set!");
}

void MailBoxRecordSet::PopRows (int oldSize)
{
	Assert ("We should never pop rows in the mailbox record set!");
}

bool MailBoxRecordSet::IsEmpty () const
{
	if (IsValid ())
	{
		for (unsigned int i = 0; i < _scriptState.size (); i++)
		{
			Mailbox::ScriptState state (_scriptState [i]);
			if (state.IsNext () || state.IsFromFuture () || state.IsMissing ())
				return false;
		}
	}
	return true;
}

bool MailBoxRecordSet::IsDefaultSelection () const
{
	for (unsigned int i = 0; i < _scriptState.size (); i++)
	{
		Mailbox::ScriptState state (_scriptState [i]);
		if (state.IsNext ())
			return true;
	}
	return false;
}

unsigned MailBoxRecordSet::GetDefaultSelectionRow () const
{
	Assert (IsDefaultSelection ());
	unsigned int i = 0;
	for ( ; i < _scriptState.size (); i++)
	{
		Mailbox::ScriptState state (_scriptState [i]);
		if (state.IsNext ())
			break;
	}
	Assert (i < _scriptState.size ());
	return i;
}

void MailBoxRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:	// Script name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:	// State
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:	// From
		{
			Mailbox::ScriptState state (_scriptState [row]);
			if (state.IsForThisProject ())
				strncpy (buf, GetSenderName (row), bufLen);
			else
				strncpy (buf, UnknownName, bufLen);
		}
		break;
	case 3:	// Timestamp
		strncpy (buf, _timeStamps [row].c_str (), bufLen);
		break;
	case 4:	// Script id
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void MailBoxRecordSet::GetImage (unsigned int row, SelectState sel, int & iImage, int & iOverlay) const
{
	if (_table.IsValid ())
	{
		Mailbox::ScriptState state (_scriptState [row]);
		if (state.IsForThisProject ())
		{
			if (state.IsNext ())
				iImage = imageNextScript;
			else if (state.IsFromFuture ())
				iImage = imageAwaitingScript;
			else if (state.IsMissing ())
				iImage = imageMissingScript;
			else 
				iImage = imageIncomingScript;
		}
		else
		{
			iImage = imageRejectedScript;
		}
	}
	else
	{
		iImage = imageNone;
	}
	iOverlay = overlayNone;
}

DrawStyle MailBoxRecordSet::GetStyle (unsigned int row) const
{
	if (_table.IsValid ())
	{
		Mailbox::ScriptState state (_scriptState [row]);
		if (state.IsMissing ())
		{
			return DrawHilite;
		}
		else if (state.IsForThisProject ())
		{
			if (state.IsNext ())
				return DrawBold;
			else if (state.IsRejected ())
				return DrawGreyed;
		}
		else
		{
			return DrawGreyed;
		}
	}
	return DrawNormal;
}

void MailBoxRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:	// Script name
		{
			// In report the name column is replaced by the version column -- full script comment
			std::string version = _table.GetStringField (Table::colVersion, _ids [row]);
			MultiLine2SingleLine (version);
			out << version;
		}
		break;
	case 1:	// State
		out << _states [row];
		break;
	case 2:	// From
		out << GetSenderName (row);
		break;
	case 3:	// Time stamp
		out << _timeStamps [row];
		break;
	case 4:	// Script id
		{
			GlobalIdPack gid (_ids [row]);
			out << gid.ToString ();
		}
		break;
	}
}

//
// Folder Record Set
//

FolderRecordSet::FolderRecordSet (Table const & table, Restriction const & restrict)
	: FileRecordSet (table)
{
	if (!_table.IsValid ())
		return;

	_rootName = _table.GetRootName ();
	// Get a listing of names
	_table.QueryUniqueNames (restrict, _names, _parentIds);
	// for each <parentId, file name> pair, get the other records
	for (unsigned i = 0; i < _names.size (); ++i)
	{
		GlobalId parentId = _parentIds [i];
		std::string const & fileName = _names [i];
		UniqueName uname (parentId, fileName);

		// global id
		GlobalId gid = _table.GetIdField (Table::colId, uname);
		_ids.push_back (gid);
		// state name
		_states.push_back (_table.GetStringField (Table::colStateName, uname));
		// type
		FileType type (_table.GetNumericField (Table::colType, uname));
		_types.push_back (type);
		// state value
		FileState state (_table.GetNumericField (Table::colState, uname));
		_fileState.push_back (state);
		// date modified
		FileTime fTime;
		if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), uname))
			_dateModified.push_back (fTime);
		// size
		_size.push_back (_table.GetNumericField (Table::colSize, uname));
		// read-only attribute
		_isReadOnly.push_back (_table.GetNumericField (Table::colReadOnly, uname) == 1);
	}
	UpdatePaths ();
	_table.ClearFileCache ();
	InitColumnHeaders (Column::BrowsingFiles);
}

void FolderRecordSet::Refresh (unsigned int row)
{
	Assert (_table.IsValid ());
	Assert (row != -1 && row < _ids.size ());
	if (_ids [row] != gidInvalid)
	{
		// We have valid global id -- use it
		GlobalId gid = _ids [row];
		std::string newName (_table.GetStringField (Table::colName, gid));
		_names [row] = newName;
		_parentIds [row] = _table.GetIdField (Table::colParentId, gid);
		_states [row] = _table.GetStringField (Table::colStateName, gid);
		FileType type (_table.GetNumericField (Table::colType, gid));
		_types [row] = type;
		FileState state (_table.GetNumericField (Table::colState, gid));
		_fileState [row] = state;
		if (state.IsNone ())
			_ids [row] = gidInvalid;	// File no longer in project
		FileTime fTime;
		if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), gid))
			_dateModified [row] = fTime;
		_size [row] = _table.GetNumericField (Table::colSize, gid);
		_isReadOnly [row] = (_table.GetNumericField (Table::colReadOnly, gid) == 1);
	}
	else
	{
		// Use unique name
		UniqueName uname (_parentIds [row], _names [row]);

		std::string newState (_table.GetStringField (Table::colStateName, uname));
		_states [row] = newState;
		GlobalId gid = _table.GetIdField (Table::colId, uname);
		_ids [row] = gid;
		FileType type (_table.GetNumericField (Table::colType, uname));
		_types [row] = type;
		FileState state (_table.GetNumericField (Table::colState, uname));
		_fileState [row] = state;
		FileTime fTime;
		if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), uname))
			_dateModified [row] = fTime;
		_size [row] = _table.GetNumericField (Table::colSize, uname);
		_isReadOnly [row] = (_table.GetNumericField (Table::colReadOnly, uname) == 1);
	}
}

void FolderRecordSet::CommitUpdate (bool delayRefresh)
{
	if (!_isValid)	//	just check our own state (and not the table's)
	{
		//	this is a stale record set, so there's nothing to do
		Assert (_change.size () == 0);
		return;
	}

	// Special commit update for the folder record set
	// We attempt to selectively update changed rows by querying
	// folder table for new file data. If the change vector contains
	// the change 'refresh all' or 'add row' or 'delete row,' we abort selective
	// update and perform complete update, which will allocate a new record set
	// replacing this one
	if (_change.size () != 0)
	{
		if (delayRefresh)
		{
			//	No selective updates possible during delayed refresh - just mark this
			//	record set invalid and tell our observer to refresh
			Invalidate ();
			//	Our observer will refresh, eventually
			Notify ();
			return;
		}

		Assert (_table.IsObserver (this));
		// Refresh changed rows
		for (ChangeIter iter = _change.begin (); iter != _change.end (); ++iter)
		{
			if (iter->IsEdit ())
			{
				unsigned int row = iter->GetRow ();
				// Ignore invisible rows (row == -1)
				if (row != -1)
					Refresh (row);
			}
			else
			{
				// Rows were added or deleted or request to get new record set
				// Force a refresh
				Invalidate ();
				//	Our observer will refresh, eventually
				Notify ();
				return;
			}
		}
		// Now selectively notify Browse Window about rows that have
		// to be re-displayed
		Notify (_change);
		Assert (_table.IsObserver (this));
		_change.clear ();
	}
}

void FolderRecordSet::PushRow (GlobalId gid, std::string const & name)
{
	Assert (_table.IsValid ());
	if (!name.empty ())
	{
		// Pushing row by file name and parent id
		_ids.push_back (gidInvalid);
		_parentIds.push_back (gid);
	}
	else
	{
		// Pushing row by file gid
		Assert (gid != gidInvalid);
		_ids.push_back (gid);
		_parentIds.push_back (gidInvalid);
	}
	_names.push_back (name.c_str ());
	_states.push_back ("");
	_types.push_back (0);
	FileState state;
	_fileState.push_back (state);
	_dateModified.push_back (CurrentPackedTime ());
	_size.push_back (0);
}

void FolderRecordSet::PopRows (int oldSize)
{
	Assert (_table.IsValid ());
	_ids.resize (oldSize);
	_parentIds.resize (oldSize);
	_names.resize (oldSize);
	_states.resize (oldSize);
	_types.resize (oldSize);
	_fileState.resize (oldSize);
	_dateModified.resize (oldSize);
	_size.resize (oldSize);
}

int FolderRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	Assert (_table.IsValid ());
	Assert (col < ColCount ());

	switch (col)
	{
	case 0: 	// File Name
		cmpResult = CmpNames (row1, row2);
		break;
	case 1: 	// State
		cmpResult = CmpStates (row1, row2);
		break;
	case 2: 	// File Type
		cmpResult = CmpTypes (row1, row2);
		break;
	case 3:		// Date Modified
		if (AreOfSameType (row1, row2))
			cmpResult = _dateModified [row1].Compare (_dateModified [row2]);
		else
			cmpResult = CmpMixedType (row1, row2);
		break;
	case 4:		// Size
		if (AreOfSameType (row1, row2))
		{
			if (_size [row1] < _size [row2])
				cmpResult = -1;
			else if (_size [row1] > _size [row2])
				cmpResult = 1;
		}
		else
			cmpResult = CmpMixedType (row1, row2);
		break;
	case 5: 	// Global ID
		cmpResult = CmpGids (row1, row2);
		break;
	}

	return cmpResult;
}

DegreeOfInterest FolderRecordSet::HowInteresting () const
{
	return NotInteresting;
}

void FolderRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:		// File Name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:		// State
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:		// File Type
		if (_ids [row] != gidInvalid)
			strncpy (buf, _types [row].GetName (), bufLen);
		else
			strncpy (buf, " ----", bufLen);
		break;
	case 3:		// Date Modified
		{
			PackedTimeStr str (_dateModified [row], true);	// Short format
			strncpy (buf, str.c_str (), bufLen);
		}
		break;
	case 4:		// Size
		if (!_types [row].IsFolder ())
			strncpy (buf, FormatFileSize (_size [row]).c_str (), bufLen);
		break;
	case 5:		// Global Id
		if (_ids [row] != gidInvalid)
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		else
			strncpy (buf, " ----", bufLen);
		break;
	}
	buf [bufLen - 1] = '\0';
}

std::string FolderRecordSet::GetStringField (unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:		// File Name
		return std::string (_names [row]);
		break;
	case 1:		// State
		return std::string (_states [row]);
		break;
	case 2:		// File Type
		if (_ids [row] != gidInvalid)
			return std::string (_types [row].GetName ());
		else
			return std::string (" ----");
		break;
	case 3:		// Date Modified
		{
			PackedTimeStr time (_dateModified [row], true);	// Short format
			return time.ToString ();
		}
		break;
	case 4:		// Size
		if (!_types [row].IsFolder ())
			return FormatFileSize (_size [row]);
		break;
	case 5:		// Global Id
		if (_ids [row] != gidInvalid)
		{
			GlobalIdPack gid (_ids [row]);
			return gid.ToString ();
		}
		else
			return std::string (" ----");
		break;
	}
	return std::string ();
}

void FolderRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	if (_table.IsValid ())
	{
		FileState state = _fileState [row];
		if (_types [row].IsFolder ())
		{
			iImage = state.IsNone ()? imageFolder: imageFolderIn;
		}
		else
		{
			if (state.IsCheckedIn ())
				iImage = imageCheckedIn;
			else if (state.IsNew ())
				iImage = imageNewFile;
			else if (state.IsRelevantIn (Area::Original))
			{
				if (state.IsPresentIn (Area::Project))
					iImage = imageCheckedOut;
				else
					iImage = imageDeleted;
			}
			else
				iImage = imageNone;

			if (state.IsRelevantIn (Area::Synch))
			{
				if (state.IsPresentIn (Area::Synch))
				{
					if (state.IsPresentIn (Area::PreSynch))
						iOverlay = overlaySynch;
					else
						iOverlay = overlaySynchNew;
				}
				else if (state.IsSoDelete ())
					iOverlay = overlaySynchDelete;
				else
					iOverlay = overlayNone; // Removed
			}
			else if (state.IsCheckedOutByOthers ())
				iOverlay = overlayLocked;
			else
				iOverlay = overlayNone;
		}
	}
	else
	{
		iImage = imageWait;
		iOverlay = overlayNone;
	}
}

void FolderRecordSet::BeginNewItemEdit ()
{
	Assert (_table.IsValid ());
	_names.push_back ("Type new folder name");
	_parentIds.push_back (gidInvalid);
	// state name
	_states.push_back ("New");
	// global id
	_ids.push_back (gidInvalid);
	// type
	FolderType type;
	_types.push_back (type);
	// state value
	StateBrandNew newState;
	_fileState.push_back (newState.GetValue ());
	_dateModified.push_back (CurrentPackedTime ());
	_size.push_back (0);
}

void FolderRecordSet::AbortNewItemEdit ()
{
	Assert (_table.IsValid ());
	_names.pop_back ();
	_parentIds.pop_back ();
	// state name
	_states.pop_back ();
	// global id
	_ids.pop_back ();
	// type
	_types.pop_back ();
	// state value
	_fileState.pop_back ();
	_dateModified.pop_back ();
	_size.pop_back ();
}

void FolderRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:		// File Name
		out << _names [row].c_str ();;
		break;
	case 1:		// State
		out << _states [row].c_str ();
		break;
	case 2:		// File Type
		out << _types [row].GetName ();
		break;
	case 3:		// Date Modified
		{
			PackedTimeStr str (_dateModified [row], true);	// Short format
			out << str.c_str ();
		}
		break;
	case 4:		// Size
		if (_types [row].IsFolder ())
		{
			out << "    ";
		}
		else
		{
			std::string str (FormatFileSize (_size [row]));
			out << str.c_str ();
		}
		break;
	case 5:		// Global Id
		if (_ids [row] != gidInvalid)
		{
			GlobalIdPack gid (_ids [row]);
			out << gid.ToString ();
		}
		else
		{
			out << "----";
		}
		break;
	}
}

bool FolderRecordSet::IsEqual (RecordSet const * recordSet) const
{
	if (recordSet->GetTableId () != GetTableId ())
		return false;

	FolderRecordSet const * old = dynamic_cast<FolderRecordSet const *>(recordSet);
	Assert (old != 0);
	if (_ids.size () != old->_ids.size ())
		return false;
	// Compare rows
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		if (_ids [i] != old->_ids [i])
			return false;
		// Our file view is case sensitive as file names go
		if (!IsCaseEqual (_names [i], old->_names [i]))
			return false;
		if (!IsNocaseEqual (_states [i], old->_states [i]))
			return false;
		if (_parentIds [i] != old->_parentIds [i])
			return false;
		if (!_types [i].IsEqual (old->_types [i]))
			return false;
		if (!_fileState [i].IsEqual (old->_fileState [i]))
			return false;
		if (_dateModified [i].Compare (old->_dateModified [i]) != 0)
			return false;
		if (_size [i] != old->_size [i])
			return false;
	}
	return true;
}

void FolderRecordSet::Verify () const
{
	if (!_table.IsValid ())
		return;

	std::vector<unsigned int> expectedReadOnly;
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		FileType type = _types [i];
		if (type.IsFolder ())
			continue;

		FileState state = _fileState [i];
		if (state.IsCheckedIn () && !_isReadOnly [i])
		{
			// Checked in file -- has to have read-only attribute
			expectedReadOnly.push_back (i);
			if (expectedReadOnly.size () == 5) // don't show more than a few files by name
				break;
		}
	}
	if (expectedReadOnly.empty ())
		return;

	std::string info;
	if (expectedReadOnly.size () < 5)
	{
		info += "The read-only attribute of the following checked-in files:\n\n";
		for (unsigned int j = 0; j < expectedReadOnly.size (); ++j)
		{
			info += _names [expectedReadOnly [j]];
			info += "\n";
		}
		info += "\nhas been changed outside of Code Co-op.\n"
		        "Run Project Repair to correct this problem.";
	}
	else
	{
		info += "The read-only attribute of a number of the checked-in files\n"
			    "has been changed outside of Code Co-op.\n"
		        "Run Project Repair to correct this problem.";
	}
	TheOutput.Display (info.c_str ());
}

//
// CheckInRecordSet
//

CheckInRecordSet::CheckInRecordSet (Table const & table, Restriction const & restrict)
	: FileRecordSet (table)
{
	if (!_table.IsValid ())
		return;

	// Get a listing of file ids
	_table.QueryUniqueIds (restrict, _ids);
	int count = _ids.size ();
	// for each id, get the other records
	for (int i = 0; i < count; i++)
	{
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		_parentIds.push_back (_table.GetIdField (Table::colParentId, _ids [i]));
		// type
		FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [i]));
		_types.push_back (type);
		FileState state (_table.GetNumericField (Table::colState, _ids [i]));
		_fileState.push_back (state);
		// date modified
		FileTime fTime;
		if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), _ids [i]))
			_dateModified.push_back (fTime);

	}
	UpdatePaths ();
	InitColumnHeaders (Column::Checkin);
}

DegreeOfInterest CheckInRecordSet::HowInteresting () const
{
	Assert (IsValid ());
	return IsEmpty () ? NotInteresting : Interesting;
}

void CheckInRecordSet::Refresh (unsigned int row)
{
	Assert (_table.IsValid ());
	std::string newName (_table.GetStringField (Table::colName, _ids [row]));
	_names [row] = newName;
	std::string newState (_table.GetStringField (Table::colStateName, _ids [row]));
	_states [row] = newState;
	_parentIds [row] = _table.GetIdField (Table::colParentId, _ids [row]);
	FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [row]));
	_types [row] = type;
	FileState state (_table.GetNumericField (Table::colState, _ids [row]));
	_fileState [row] = state;
	FileTime fTime;
	if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), _ids [row]))
		_dateModified [row] = fTime;
	// add parent id to cache
	std::string parentPath = _table.GetStringField (Table::colParentPath, _parentIds [row]);
	_pathCache.AddString (_parentIds [row], parentPath);
}

void CheckInRecordSet::PushRow (GlobalId gid, std::string const & name)
{
	Assert (_table.IsValid ());
	Assert (gid != gidInvalid);
	_ids.push_back (gid);
	_parentIds.push_back (gidInvalid);
	_names.push_back (name.c_str ());
	_states.push_back ("");
	_types.push_back (0);
	FileState state;
	_fileState.push_back (state);
	_dateModified.push_back (CurrentPackedTime ());
}

void CheckInRecordSet::PopRows (int oldSize)
{
	Assert (_table.IsValid ());
	_ids.resize (oldSize);
	_parentIds.resize (oldSize);
	_names.resize (oldSize);
	_states.resize (oldSize);
	_types.resize (oldSize);
	_fileState.resize (oldSize);
	_dateModified.resize (oldSize);
}

int CheckInRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	if (_table.IsValid ())
	{
		Assert (col < ColCount ());
		switch (col)
		{
		case 0: 	// File Name
			cmpResult = CmpNames (row1, row2);
			break;
		case 1: 	// Change
			cmpResult = CmpChanged (row1, row2);
			break;
		case 2: 	// State
			cmpResult = CmpStates (row1, row2);
			break;
		case 3: 	// Type
			cmpResult = CmpTypes (row1, row2);
			break;
		case 4:		// Date Modified
			cmpResult = _dateModified [row1].Compare (_dateModified [row2]);
			break;
		case 5: 	// Path
			cmpResult = CmpPaths (row1, row2);
			break;
		case 6: 	// Global ID
			cmpResult = CmpGids (row1, row2);
			break;
		}
	}
	return cmpResult;
}

void CheckInRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:		// Name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:		// Change
		{
			FileState state = _fileState [row];
			if (state.IsMoved ())
				strcpy (buf, "Moved");
			else if (state.IsRenamed ())
				strcpy (buf, "Renamed");
			else if (!state.IsPresentIn (Area::Project))
				strcpy (buf, "Deleted");
			else if (state.IsNew ())
				strcpy (buf, "Created");
			else if (state.IsCoDiff ())
				strcpy (buf, "Edited");
			else if (state.IsTypeChanged ())
				strcpy (buf, "Type Changed");
			else
				strcpy (buf, " ");
		}
		break;
	case 2:		// State
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 3:		// Type
		strncpy (buf, _types [row].GetName (), bufLen);
		break;
	case 4:		// Date Modified
		{
			PackedTimeStr str (_dateModified [row], true);	// Short format
			strncpy (buf, str.c_str (), bufLen);
		}
		break;
	case 5:		// Path
		strcpy (buf, GetParentPath (row));
		break;
	case 6:		// Global id
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void CheckInRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	Assert (_table.IsValid ());

	if (_types [row].IsFolder ())
		iImage = imageFolderIn;
	else
	{
		FileState state = _fileState [row];
		if (state.IsNew ())
			iImage = imageNewFile;
		else
		{
			Assert (state.IsRelevantIn (Area::Original));
			if (state.IsMerge ())
				iImage = imageMerge;
			else if (state.IsPresentIn (Area::Project))
				iImage = imageCheckedOut;
			else if (state.IsCoDelete ())
				iImage = imageDeleted;
			else
				iImage = imageNone; 
		}

		if (state.IsRelevantIn (Area::Synch))
		{
			if (state.IsPresentIn (Area::Synch))
			{
				if (state.IsPresentIn (Area::PreSynch))
					iOverlay = overlaySynch;
				else
					iOverlay = overlaySynchNew;
			}
			else if (state.IsSoDelete ())
				iOverlay = overlaySynchDelete;
			else
				iOverlay = overlayNone;
		}
		else if (state.IsCheckedOutByOthers ())
			iOverlay = overlayLocked;
		else
			iOverlay = overlayNone;
	}
}

void CheckInRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:		// File name
		out << _names [row].c_str ();
		break;
	case 1:		// Change
		{
			FileState state = _fileState [row];
			if (state.IsCoDiff () || !state.IsPresentIn (Area::Project))
				out << "Edited";
			else if (state.IsMoved ())
				out << "Moved";
			else if (state.IsRenamed ())
				out << "Renamed";
			else if (state.IsTypeChanged ())
				out << "Type Changed";
			else
				out << ' ';
		}
		break;
	case 2:		// State
		out << _states [row].c_str ();
		break;
	case 3:		// Type
		out << _types [row].GetName ();
		break;
	case 4:		// Date Modified
		{
			PackedTimeStr str (_dateModified [row], true);	// Short format
			out << str.c_str ();
		}
		break;
	case 5:
		out << GetParentPath (row);
		break;
	case 6:
		out << GlobalIdPack (_ids [row]);
		break;
	}
}

//
// SynchRecordSet
//

SynchRecordSet::SynchRecordSet (Table const & table, Restriction const & restrict)
	: FileRecordSet (table)
{
	if (!_table.IsValid ())
		return;

	// Get a listing of script ids
	_table.QueryUniqueIds (restrict, _ids);
	int count = _ids.size ();
	// for each script id, get the other two records
	for (int i = 0; i < count; i++)
	{
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		_parentIds.push_back (_table.GetIdField (Table::colParentId, _ids [i]));
		// type
		FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [i]));
		_types.push_back (type);
		FileState state (_table.GetNumericField (Table::colState, _ids [i]));
		_fileState.push_back (state);
	}
	UpdatePaths ();
	InitColumnHeaders (Column::Synch);
}

void SynchRecordSet::Refresh (unsigned int row)
{
	Assert (_table.IsValid ());
	std::string newName (_table.GetStringField (Table::colName, _ids [row]));
	_names [row] = newName;
	std::string newState (_table.GetStringField (Table::colStateName, _ids [row]));
	_states [row] = newState;
	_parentIds [row] = _table.GetIdField (Table::colParentId, _ids [row]);
	FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [row]));
	_types [row] = type;
	FileState state (_table.GetNumericField (Table::colState, _ids [row]));
	_fileState [row] = state;
	// add parent id to cache
	std::string parentPath = _table.GetStringField (Table::colParentPath, _parentIds [row]);
	_pathCache.AddString (_parentIds [row], parentPath);
}

void SynchRecordSet::PushRow (GlobalId gid, std::string const & name)
{
	// Synch area is the only record set that can push/pop row while
	// its table is invalid -- this happens during unpacking full synch.
	Assert (gid != gidInvalid);
	_ids.push_back (gid);
	_parentIds.push_back (gidInvalid);
	_names.push_back (name.c_str ());
	_states.push_back ("");
	_types.push_back (0);
	FileState state;
	_fileState.push_back (state);
}

void SynchRecordSet::PopRows (int oldSize)
{
	// Synch area is the only record set that can push/pop row while
	// its table is invalid -- this happens during unpacking full synch.
	_ids.resize (oldSize);
	_parentIds.resize (oldSize);
	_names.resize (oldSize);
	_states.resize (oldSize);
	_types.resize (oldSize);
	_fileState.resize (oldSize);
}

int SynchRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	if (_table.IsValid ())
	{
		Assert (col < ColCount ());
		switch (col)
		{
		case 0: 	// File Name
			cmpResult = CmpNames (row1, row2);
			break;
		case 1: 	// Change
			if (_states [row1] == "Edited" && _states [row2] == "Edited")
				cmpResult = CmpChanged (row1, row2);
			else
				cmpResult = CmpStates (row1, row2);
			break;
		case 2: 	// Type
			cmpResult = CmpTypes (row1, row2);
			break;
		case 3: 	// Path
			cmpResult = CmpPaths (row1, row2);
			break;
		case 4: 	// Global ID
			cmpResult = CmpGids (row1, row2);
			break;
		}
	}
	return cmpResult;
}

void SynchRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:		// Name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:		// Change
		if (_states [row] == "Edited" || _states [row] == "Restored")
		{
			// Elaborate on those sync kinds
			FileState state = _fileState [row];
			if (state.IsMergeConflict ())
				strcpy (buf, "Conflict");
			else if (state.IsMerge ())
				strcpy (buf, "Merged");
			else if (state.IsCoDiff ())
				strcpy (buf, "Edited");
			else if (state.IsMoved ())
				strcpy (buf, "Moved");
			else if (state.IsRenamed ())
				strcpy (buf, "Renamed");
			else if (state.IsTypeChanged ())
				strcpy (buf, "Type Changed");
			else
				strcpy (buf, " ");
		}
		else if (_states [row] == "Deleted")
		{
			FileState state = _fileState [row];
			if (state.IsSoDelete ())
				strcpy (buf, "Deleted");
			else
				strcpy (buf, "Uncontrolled");
		}
		else
			strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:		// Type
		strncpy (buf, _types [row].GetName (), bufLen);
		break;
	case 3:		// Path
		strcpy (buf, GetParentPath (row));
		break;
	case 4:		// Global id
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void SynchRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	Assert (_table.IsValid ());

	if (_types [row].IsFolder ())
		iImage = imageFolderIn;
	else
	{
		FileState state = _fileState [row];
		if (state.IsCheckedIn ())
			iImage = imageCheckedIn;
		else if (state.IsNew ())
			iImage = imageNewFile;
		else if (state.IsRelevantIn (Area::Original))
		{
			if (state.IsMergeConflict ())
				iImage = imageMergeConflict;
			else if (state.IsMerge ())
				iImage = imageMerge;
			else if (state.IsPresentIn (Area::Project))
				iImage = imageCheckedOut;
			else
				iImage = imageDeleted;
		}
		else
			iImage = imageNone;

		if (state.IsRelevantIn (Area::Synch) && _states [row] != "Restored")
		{
			if (state.IsPresentIn (Area::Synch))
			{
				if (state.IsPresentIn (Area::PreSynch))
					iOverlay = overlaySynch;// Synched file
				else
					iOverlay = overlaySynchNew;
			}
			else if (state.IsSoDelete ())
				iOverlay = overlaySynchDelete;
			else // Removed
				iOverlay = overlayNone;
		}
		else
			iOverlay = overlayNone;
	}
}

DrawStyle SynchRecordSet::GetStyle (unsigned int row) const
{
	FileState state = _fileState [row];
	if (state.IsMergeConflict ())
		return DrawHilite;

	return DrawNormal;
}

void SynchRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:		// Name
		out << _names [row];
		break;
	case 1:		// Change
		if (_states [row] == "Edited")
		{
			FileState state = _fileState [row];
			if (state.IsMergeConflict ())
				out << "Conflict";
			else if (state.IsMerge ())
				out << "Merged";
			else if (state.IsCoDiff ())
				out << "Edited";
			else if (state.IsMoved ())
				out << "Moved";
			else if (state.IsRenamed ())
				out << "Renamed";
			else if (state.IsTypeChanged ())
				out << "Type Changed";
			else
				out << " ";
		}
		else if (_states [row] == "Deleted")
		{
			FileState state = _fileState [row];
			if (state.IsSoDelete ())
				out << "Deleted";
			else
				out << "Uncontrolled";
		}
		else
			out << _states [row];
		break;
	case 2:		// Type
		out << _types [row].GetName ();
		break;
	case 3:		// Path
		out << GetParentPath (row);
		break;
	case 4:		// Global id
		{
			GlobalIdPack gid (_ids [row]);
			out << gid.ToString ();
		}
		break;
	}
}

//
// History Record Set
//

HistoryRecordSet::HistoryRecordSet (Table const & table, Restriction const & restrict)
	: ScriptRecordSet (table)
{
	if (!_table.IsValid ())
		return;

	// Get a listing of script ids
	_table.QueryUniqueIds (restrict, _ids);
	unsigned count = _ids.size ();
	_names.reserve (count);
	_states.reserve (count);
	_scriptState.reserve (count);
	_timeStamps.reserve (count);
	UpdateSenders ();
	// for each script id, get the other fields
	for (unsigned i = 0; i < count; ++i)
	{
		GlobalId scriptId = _ids [i];
		_names.push_back (_table.GetStringField (Table::colName, scriptId));
		_states.push_back (_table.GetStringField (Table::colStateName, scriptId));
		History::ScriptState state (_table.GetNumericField (Table::colState, scriptId));
		if (restrict.IsInteresting (scriptId))
			state.SetInteresting (true);
		_scriptState.push_back (state.GetValue ());
		_timeStamps.push_back (_table.GetStringField (Table::colTimeStamp, scriptId));
	}
	InitColumnHeaders (Column::History);
}

char const * HistoryRecordSet::GetSenderName (unsigned int row) const
{
	Assert (_table.IsValid ());
	Assert (row < _ids.size ());
	History::ScriptState state (GetState (row));
	if (state.IsInventory () && !state.IsMyInventory ())
		return "Administrator";

	return ScriptRecordSet::GetSenderName (row);
}

DrawStyle HistoryRecordSet::GetStyle (unsigned int row) const
{
	Assert (_table.IsValid ());
	Assert (row < _ids.size ());
	History::ScriptState state (_scriptState [row]);
	if (state.IsRejected () || !state.IsInteresting ())
		return DrawGreyed;
	else if (state.IsToBeRejected ())
		return DrawHilite;
	else if (state.IsLabel () || state.IsCurrent ())
		return DrawBold;

	return DrawNormal;
}

void HistoryRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:
		strncpy (buf, GetSenderName (row), bufLen);
		break;
	case 2:
		strncpy (buf, _timeStamps [row].c_str (), bufLen);
		break;
	case 3:
		if (_ids [row] != gidInvalid)
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	case 4:
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	}
	buf [bufLen - 1] = '\0';
}

void HistoryRecordSet::GetImage (unsigned int row, SelectState sel, int & iImage, int & iOverlay) const
{
	Assert (_table.IsValid ());

	History::ScriptState state (_scriptState [row]);
	if (state.IsCurrent ())
		iImage = imageVersionCurrent;
	else if (state.IsLabel ())
		iImage = imageLabel;
	else if (state.IsRejected ())
	{
		if (sel.IsRange ())
			iImage = (row & 1) ? imageVersionRange1: imageVersionRange2; // alternate
		else			
			iImage = imageLast;	// No image - imageLast is an index of the non existing image
	}
	else if (sel.IsRange ())
		iImage = (row & 1) ? imageVersionRange1: imageVersionRange2; // alternate
	else
		iImage = (row & 1) ? imageVersion1: imageVersion2; // alternate
	iOverlay = overlayNone;
}

void HistoryRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:
		{
			// In report the name column is replaced by the version column -- full script comment
			std::string version (_table.GetStringField (Table::colVersion, _ids [row]));
			MultiLine2SingleLine (version);
			out << version;
		}
		break;
	case 1:
		out << GetSenderName (row);
		break;
	case 2:
		out << _timeStamps [row];
		break;
	case 3:
		if (_ids [row] != gidInvalid)
			out << GlobalIdPack (_ids [row]);
		break;
	case 4:
		out << _states [row];
		break;
	}
}

//
// Script Details Record Set
//

ScriptDetailsRecordSet::ScriptDetailsRecordSet (Table const & table, Restriction const & restrict)
	: FileRecordSet (table)
{
	// Get a listing of file ids
	_table.QueryUniqueIds (restrict, _ids);
	unsigned count = _ids.size ();
	// For each file id, get the other fields
	for (unsigned i = 0; i < count; i++)
	{
		// File name
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		// Change
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		// Status value
		MergeStatus status (_table.GetNumericField (Table::colState, _ids [i]));
		_mergeStatus.push_back (status);
		// Parent ids - used to display project path
		_parentIds.push_back (_table.GetIdField (Table::colParentId, _ids [i]));
		// File type
		FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [i]));
		_types.push_back (type);
	}
	UpdatePaths ();
	InitColumnHeaders (Column::ScriptDetails);
}

void ScriptDetailsRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0:		// File name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:		// Change
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:		// Merge Status
		strncpy (buf, _mergeStatus [row].GetStatusName (), bufLen);
		break;
	case 3:		// Type
		strncpy (buf, _types [row].GetName (), bufLen);
		break;
	case 4:		// Project path
		strcpy (buf, GetParentPath (row));
		break;
	case 5:		// Global id
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void ScriptDetailsRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	MergeStatus mergeStatus = _mergeStatus [row];
	if (mergeStatus.IsValid ())
	{
		if (mergeStatus.IsCreated ())
			iOverlay = overlaySynchNew;
		else if (mergeStatus.IsDeletedAtSource ())
			iOverlay = overlaySynchDelete;
		else
			iOverlay = overlayNone;

		if (mergeStatus.IsConflict ())
			iImage = imageMergeConflict;
		else if (mergeStatus.IsDifferent () || mergeStatus.IsMerged ())
			iImage = imageMerge;
		else if (mergeStatus.IsDeletedAtTarget ())
			iImage = imageDeleted;
		else
			iImage = imageCheckedOut;
	}
	else
	{
		if (_states [row] == "Created")
			iOverlay = overlaySynchNew;
		else if (_states [row] == "Deleted")
			iOverlay = overlaySynchDelete;
		else
			iOverlay = overlayNone;

		if (_types [row].IsFolder ())
			iImage = imageFolderIn;
		else
			iImage = imageCheckedIn;
	}
}

bool ScriptDetailsRecordSet::IsEqual (RecordSet const * recordSet) const
{
	if (recordSet->GetTableId () != GetTableId ())
		return false;

	ScriptDetailsRecordSet const * old = dynamic_cast<ScriptDetailsRecordSet const *>(recordSet);
	Assert (old != 0);
	if (_ids.size () != old->_ids.size ())
		return false;
	// Compare rows
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		if (_ids [i] != old->_ids [i])
			return false;
		// Our file view is case sensitive as file names go
		if (!IsCaseEqual (_names [i], old->_names [i]))
			return false;
		if (!IsNocaseEqual (_states [i], old->_states [i]))
			return false;
		if (!_mergeStatus [i].GetValue () != old->_mergeStatus [i].GetValue ())
			return false;
		if (_parentIds [i] != old->_parentIds [i])
			return false;
		if (!_types [i].IsEqual (old->_types [i]))
			return false;
	}
	return true;
}

void ScriptDetailsRecordSet::Refresh (unsigned int row)
{
	// REVISIT: implementation
}

int ScriptDetailsRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	Assert (_table.IsValid ());
	Assert (col < ColCount ());

	switch (col)
	{
	case 0: 	// File Name
		cmpResult = CmpNames (row1, row2);
		break;
	case 1: 	// Change
		cmpResult = RecordSet::CmpStates (row1, row2);
		break;
	case 2:		// Merge Status
		Assert (row1 < _mergeStatus.size ());
		Assert (row2 < _mergeStatus.size ());
		{
			MergeStatus row1Status = _mergeStatus [row1];
			MergeStatus row2Status = _mergeStatus [row2];
			if (row1Status.GetValue () != row2Status.GetValue ())
			{
				if (row1Status.IsIdentical ())
					cmpResult = 1;
				else if (row2Status.IsIdentical ())
					cmpResult = -1;
				else if (row1Status.IsDeletedAtSource () || row1Status.IsDeletedAtTarget ())
					cmpResult = 1;
				else if (row2Status.IsDeletedAtSource () || row2Status.IsDeletedAtTarget ())
					cmpResult = -1;
				else if (row1Status.IsCreated ())
					cmpResult = 1;
				else
					cmpResult = -1;
			}
		}
		break;
	case 3: 	// File Type
		cmpResult = CmpTypes (row1, row2);
		break;
	case 4:		// Project path
		cmpResult = CmpPaths (row1, row2);
		break;
	case 5: 	// Global ID
		cmpResult = CmpGids (row1, row2);
		break;
	}

	return cmpResult;
}

DrawStyle ScriptDetailsRecordSet::GetStyle (unsigned int row) const
{
	MergeStatus mergeStatus = _mergeStatus [row];
	if (mergeStatus.IsConflict ())
		return DrawHilite;

	return DrawNormal;
}

void ScriptDetailsRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (_table.IsValid ());
	Assert (col < ColCount ());

	switch (col)
	{
	case 0: 	// File Name
		out << _names [row];
		break;
	case 1: 	// Change
		out << _states [row];
		break;
	case 2:		// Merge Status
		out << _mergeStatus [row].GetStatusName ();
		break;
	case 3: 	// File Type
		out << _types [row].GetName ();
		break;
	case 4:		// Project path
		out << GetParentPath (row);
		break;
	case 5: 	// Global ID
		{
			GlobalIdPack gid (_ids [row]);
			out << gid.ToString ();
		}
		break;
	}
}

//
// Merge Details Record Set
//

MergeDetailsRecordSet::MergeDetailsRecordSet (Table const & table, Restriction const & restrict)
	: FileRecordSet (table)
{
	// Get a listing of file ids
	_table.QueryUniqueIds (restrict, _ids);
	unsigned count = _ids.size ();
	// For each file id, get the other fields
	for (unsigned i = 0; i < count; i++)
	{
		// File name
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		// Merge status value
		MergeStatus status (_table.GetNumericField (Table::colState, _ids [i]));
		_mergeStatus.push_back (status);
		// File status name
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		// File patent id
		_parentIds.push_back (_table.GetIdField (Table::colParentId, _ids [i]));
		// File type
		FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [i]));
		_types.push_back (type);
		// Merge target path
		_targetPaths.push_back (_table.GetStringField (Table::colTargetPath, _ids [i]));
	}
	UpdatePaths ();
	InitColumnHeaders (Column::MergeDetails);
}

void MergeDetailsRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_table.IsValid ());

	switch (col)
	{
	case 0: 	// Source File Name
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1: 	// Merge Status
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:		// Source Path
		strcpy (buf, GetParentPath (row));
		break;
	case 3:		// Target Path
		strncpy (buf, _targetPaths [row].c_str (), bufLen);
		break;
	case 4: 	// Global ID
		{
			GlobalIdPack gid (_ids [row]);
			strncpy (buf, gid.ToString ().c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void MergeDetailsRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	MergeStatus mergeStatus = _mergeStatus [row];
	if (mergeStatus.IsIdentical ())
	{
		iImage = imageCheckedIn;
		iOverlay = overlayNone;
	}
	else
	{
		if (mergeStatus.IsCreated ())
			iOverlay = overlaySynchNew;
		else if (mergeStatus.IsDeletedAtSource ())
			iOverlay = overlaySynchDelete;
		else
			iOverlay = overlayNone;

		if (mergeStatus.IsConflict ())
			iImage = imageMergeConflict;
		else if (mergeStatus.IsDifferent () || mergeStatus.IsMerged ())
			iImage = imageMerge;
		else if (mergeStatus.IsDeletedAtTarget ())
			iImage = imageDeleted;
		else
			iImage = imageCheckedOut;
	}
}

bool MergeDetailsRecordSet::IsEqual (RecordSet const * recordSet) const
{
	if (recordSet->GetTableId () != GetTableId ())
		return false;

	MergeDetailsRecordSet const * old = dynamic_cast<MergeDetailsRecordSet const *>(recordSet);
	Assert (old != 0);
	if (_ids.size () != old->_ids.size ())
		return false;
	// Compare rows
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		if (_ids [i] != old->_ids [i])
			return false;
		// Our file view is case sensitive as file names go
		if (!IsCaseEqual (_names [i], old->_names [i]))
			return false;
		if (!IsNocaseEqual (_states [i], old->_states [i]))
			return false;
		if (_parentIds [i] != old->_parentIds [i])
			return false;
		if (!_types [i].IsEqual (old->_types [i]))
			return false;
		if (!_mergeStatus [i].GetValue () != old->_mergeStatus [i].GetValue ())
			return false;
		if (!IsNocaseEqual (_targetPaths[i], old->_targetPaths [i]))
			return false;
	}
	return true;
}

void MergeDetailsRecordSet::Refresh (unsigned int row)
{
	// File name
	_names.push_back (_table.GetStringField (Table::colName, _ids [row]));
	// Status value
	MergeStatus status (_table.GetNumericField (Table::colState, _ids [row]));
	_mergeStatus.push_back (status);
	// File status name
	_states.push_back (_table.GetStringField (Table::colStateName, _ids [row]));
	// File patent id
	_parentIds.push_back (_table.GetIdField (Table::colParentId, _ids [row]));
	// File type
	FileType type = static_cast<FileType> (_table.GetNumericField (Table::colType, _ids [row]));
	_types.push_back (type);
	// Merge target path
	_targetPaths.push_back (_table.GetStringField (Table::colTargetPath, _ids [row]));
	UpdatePaths ();
}

int MergeDetailsRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	Assert (_table.IsValid ());
	Assert (col < ColCount ());

	switch (col)
	{
	case 0: 	// Source File Name
		cmpResult = CmpNames (row1, row2);
		break;
	case 1: 	// Status
		Assert (row1 < _mergeStatus.size ());
		Assert (row2 < _mergeStatus.size ());
		{
			MergeStatus row1Status = _mergeStatus [row1];
			MergeStatus row2Status = _mergeStatus [row2];
			if (row1Status.GetValue () != row2Status.GetValue ())
				cmpResult = row1Status.GetValue () < row2Status.GetValue () ? -1 : 1;
		}
		break;
	case 2:		// Source Path
		cmpResult = CmpPaths (row1, row2);
		break;
	case 3:		// Target Path
		Assert (row1 < _targetPaths.size ());
		Assert (row2 < _targetPaths.size ());
		if (AreOfSameType (row1, row2))
		{
			// Files or folders
			if (!FilePath::IsEqualDir (_targetPaths [row1], _targetPaths [row2]))
				cmpResult = FilePath::IsDirLess (_targetPaths [row1], _targetPaths [row2]) ? -1 : 1;
		}
		else
		{
			// Mixed file with folder
			cmpResult = CmpMixedType (row1, row2);
		}
		break;
	case 4: 	// Global ID
		cmpResult = CmpGids (row1, row2);
		break;
	}

	return cmpResult;
}

DrawStyle MergeDetailsRecordSet::GetStyle (unsigned int row) const
{
	MergeStatus mergeStatus = _mergeStatus [row];
	if (mergeStatus.IsConflict ())
		return DrawHilite;

	return DrawNormal;
}

void MergeDetailsRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (_table.IsValid ());
	Assert (col < ColCount ());

	switch (col)
	{
	case 0: 	// Source File Name
		out << _names [row];
		break;
	case 1: 	// Status
		out << _states [row];
		break;
	case 2:		// Source Path
		out << GetParentPath (row);
		break;
	case 3:		// Target Path
		out << _targetPaths [row];
		break;
	case 4: 	// Global ID
		{
			GlobalIdPack gid (_ids [row]);
			out << gid.ToString ();
		}
		break;
	}
}

//
// Project Record Set
//

ProjectRecordSet::ProjectRecordSet (Table const & table, Restriction const & restrict)
	: RecordSet (table)
{
	if (!_table.IsValid ())
		return;

	// _ids -- local project ids
	// _names -- project names
	// _states -- source code paths
	// _projState -- project state
	// _lastModified -- last modification date - the CmdLog.bin file modification time
	// Get a listing of local project ids
	_table.QueryUniqueIds (restrict, _ids);
	int count = _ids.size ();
	// For each local project id, get the other two records
	for (int i = 0; i < count; i++)
	{
		_names.push_back (_table.GetStringField (Table::colName, _ids [i]));
		_states.push_back (_table.GetStringField (Table::colStateName, _ids [i]));
		Project::State state (_table.GetNumericField (Table::colState, _ids [i]));
		_projState.push_back (state);
		FileTime fTime;
		if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), _ids [i]))
			_lastModified.push_back (fTime);
	}
	InitColumnHeaders (Column::Project);
}

void ProjectRecordSet::Refresh (unsigned int row)
{
	Assert (_table.IsValid ());
	std::string newName (_table.GetStringField (Table::colName, _ids [row]));
	_names [row] = newName;
	std::string newState (_table.GetStringField (Table::colStateName, _ids [row]));
	_states [row] = newState;
	Project::State state ( _table.GetNumericField (Table::colState, _ids [row]));
	_projState [row] = state;
	FileTime fTime;
	if (_table.GetBinaryField (Table::colTimeStamp, &fTime, sizeof (FileTime), _ids [row]))
		_lastModified [row] = fTime;
}

void ProjectRecordSet::PushRow (GlobalId gid, std::string const & name)
{
	Assert (gid != gidInvalid);
	_ids.push_back (gid);
	_names.push_back (name.c_str ());
	_states.push_back ("");
	Project::State state;
	_projState.push_back (state);
	_lastModified.push_back (CurrentPackedTime ());
}

void ProjectRecordSet::PopRows (int oldSize)
{
	_ids.resize (oldSize);
	_names.resize (oldSize);
	_states.resize (oldSize);
	_projState.resize (oldSize);
	_lastModified.resize (oldSize);
}

bool ProjectRecordSet::IsEqual (RecordSet const * recordSet) const
{
	if (recordSet->GetTableId () != GetTableId ())
		return false;

	ProjectRecordSet const * old = dynamic_cast<ProjectRecordSet const *>(recordSet);
	Assert (old != 0);
	if (_ids.size () != old->_ids.size ())
		return false;
	// Compare rows
	for (unsigned int i = 0; i < _ids.size (); ++i)
	{
		if (_projState [i].GetValue () != old->_projState [i].GetValue ())
			return false;
		// Compare local project ids
		if (_ids [i] != old->_ids [i])
			return false;
		// Compare last modified date
		if (_lastModified [i].Compare (old->_lastModified [i]) != 0)
			return false;
		// Compare project names
		if (_names [i] != old->_names [i])
			return false;
		// Compare project root paths
		if (_states [i] != old->_states [i])
			return false;
	}
	return true;
}

int ProjectRecordSet::CmpRows (unsigned int row1, unsigned int row2, unsigned int col) const
{
	int cmpResult = 0;
	if (_table.IsValid ())
	{
		Assert (col < ColCount ());
		switch (col)
		{
		case 0: 	// Project Name
			cmpResult = CmpNames (row1, row2);
			break;
		case 1: 	// Source Code Path
			cmpResult = CmpStates (row1, row2);
			break;
		case 2:		// Last modified date
			cmpResult = _lastModified [row1].Compare (_lastModified [row2]);
			break;
		case 3:		// Project Id
			cmpResult = CmpGids (row1, row2);
			break;
		}
	}
	return cmpResult;
}

DegreeOfInterest ProjectRecordSet::HowInteresting () const
{
	Assert (IsValid ());
	bool incomingScripts = false;
	bool checkedOutFiles = false;
	bool awaitsFullSync = false;
	bool recovery = false;
	for (std::vector<Project::State>::const_iterator iter = _projState.begin ();
			iter != _projState.end ();
			++iter)
	{
		Project::State state = *iter;
		if (!incomingScripts)
			incomingScripts = state.HasMail ();
		if (!checkedOutFiles)
			checkedOutFiles = state.HasCheckedoutFiles ();
		if (!awaitsFullSync)
			awaitsFullSync = state.IsAwatingFullsync ();
		if (!recovery)
			recovery = state.IsUnderRecovery ();
	}
	if (incomingScripts)
		return VeryInteresting;
	else if (checkedOutFiles  || awaitsFullSync || recovery)
		return Interesting;
	else
		return NotInteresting;
}

void ProjectRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:
		strncpy (buf, _names [row].c_str (), bufLen);
		break;
	case 1:
		strncpy (buf, _states [row].c_str (), bufLen);
		break;
	case 2:
		{
			PackedTimeStr str (_lastModified [row], true);	// Short format
			strncpy (buf, str.c_str (), bufLen);
		}
		break;
	case 3:
		{
			std::string id (ToString (_ids [row]));
			strncpy (buf, id.c_str (), bufLen);
		}
		break;
	}
	buf [bufLen - 1] = '\0';
}

void ProjectRecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	Project::State project = _projState [row];
	if (project.IsCurrent ())
	{
		if (project.HasCheckedoutFiles ())
			iImage = imageCurOutProj;
		else
			iImage = imageCurProject;
	}
	else
	{
		if (project.HasCheckedoutFiles ())
			iImage = imageOutProj;
		else
			iImage = imageProject;
	}
	if (project.HasMail ())
		iOverlay = overlaySynchProj;
	else if (project.IsAwatingFullsync ())
		iOverlay = overlayFullSyncProj;
	else if (project.IsUnderRecovery ())
		iOverlay = overlayRecoveryProj;
	else
		iOverlay = overlayNone;
}

void ProjectRecordSet::DumpField (std::ostream & out, unsigned int row, unsigned int col) const
{
	Assert (col < ColCount ());
	switch (col)
	{
	case 0:
		out << _names [row].c_str ();
		break;
	case 1:
		out << _states [row].c_str ();
		break;
	case 2:
		{
			PackedTimeStr str (_lastModified [row], true);	// Short format
			out << str.c_str ();
		}
		break;
	case 3:
		out <<	ToString (_ids [row]).c_str ();
		break;
	}
}

// Wiki Record Set

WikiRecordSet::WikiRecordSet (Table const & table)
	: RecordSet (table)
{
	if (!_table.IsValid ())
		return;
	Restriction restriction;
	table.QueryUniqueNames (restriction, _urls);
	// Current wiki root directory path
	_rootName = _table.GetRootName ();
}

std::string WikiRecordSet::GetStringField (unsigned int row, unsigned int col) const
{
	Assert (col == 0);
	Assert (row < _urls.size ());
	return _urls [row];
}


//
// Empty Record Set -- contains information displayed during inter-process command execution or
// when Code Co-op is not visiting any project or when project is awaiting full sync script
//

EmptyRecordSet::EmptyRecordSet (Table const & table)
	: RecordSet (table)
{
	Assert (_table.IsValid ());
	_ids.push_back (gidInvalid);
	_names.push_back (_table.GetStringField (Table::colName, gidInvalid));
	InitColumnHeaders (Column::NotInProject);
}

void EmptyRecordSet::CopyStringField (unsigned int row, unsigned int col, unsigned int bufLen, char * buf) const
{
	Assert (col < ColCount ());
	Assert (_ids.size () == 1);
	strncpy (buf, _names [0].c_str (), bufLen);
	buf [bufLen - 1] = '\0';
}
