//----------------------------------
// (c) Reliable Software 1998 - 2009
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CmdLineSelection.h"
#include "Table.h"

#include <Dbg/Out.h>

PathParser::PathParser (Directory const & folder)
	: _folder (folder)
{
	_folder.GotoRoot ();
	_root.Change (_folder.GetCurrentPath ());
}

UniqueName const * PathParser::Convert (char const * fullPath)
{
	dbg << "PathParser::Convert";
	dbg << fullPath << std::endl;
	FilePath path (fullPath);
	// Must be in the project tree
	if (_root.IsDirStrEmpty () || !path.HasPrefix (_root))
		return 0;

	// While not in the current folder, go up
	while (!path.HasPrefix (_folder.GetCurrentPath ()))
		_folder.Up ();

	char const * relativePath = _folder.GetRelativePath (path.GetDir ());
	char const * curSegment;

	// Now go down
	PartialPathSeq seq (relativePath);
	if (seq.AtEnd ())
	{
		// Project root folder -- create special unique name
		_uname.Init (gidInvalid, std::string ());
	}
	else
	{
		for (;;)
		{
			curSegment = seq.GetSegment ();
			seq.Advance ();
			if (seq.AtEnd ()) // it was the last segment
				break;

			_folder.Down (curSegment);
			if (_folder.GetCurrentId () == gidInvalid)
			{
				_folder.Up ();
				curSegment = _folder.GetRelativePath (path.GetDir ());
				break;
			}
		}

		// curSegment points the last path segment - file or folder name
		// or a relative path starting at the first folder not in project
		GlobalId idParent = _folder.GetCurrentId ();
		_uname.Init (idParent, curSegment);
	}

	Assert (_uname.IsValid ());
	return &_uname;
}

void CmdLineSelection::SetSelection (PathSequencer & sequencer)
{
	_nameSet.clear ();
	_names.clear ();
	PathParser parser (_folder);
	for (; !sequencer.AtEnd (); sequencer.Advance ())
	{
		// Convert full path into UniqueName
		UniqueName const * uname = parser.Convert (sequencer.GetFilePath ());
		if (uname != 0)
		{
			_names.push_back (*uname);
			_nameSet.insert (_names.back ());
		}
	}
}

void CmdLineSelection::SetSelection (GidList const & ids)
{
	_ids.clear ();
	std::copy (ids.begin (), ids.end (), std::back_inserter (_ids));
}

void CmdLineSelection::Clear ()
{
	_nameSet.clear ();
	_names.clear ();
	_ids.clear ();
	_recordSet.reset (0);
}

RecordSet const * CmdLineSelection::GetRecordSet (Table::Id tableId) const
{
	// Pass to the restriction command selection - names and/or ids
	Restriction  restrict (&_names, &_ids);
	// Dont't observe!
	_recordSet = _tableProvider.Query (tableId, restrict);
	return _recordSet.get ();
}

void CmdLineSelection::DumpRecordSet (std::ostream & out, Table::Id tableId, bool allRows)
{
	if (_recordSet.get () == 0)
		GetRecordSet (tableId);
	Assert (tableId == _recordSet->GetTableId ());
	int rowCount = _recordSet->RowCount ();
	_recordSet->DumpColHeaders (out);
	// In command line mode all rows are always selected
	for (int i = 0; i < rowCount; ++i)
		_recordSet->DumpRow (out, i);
}

void CmdLineSelection::GetRows (std::vector<unsigned> & rows, Table::Id tableId) const
{
	Assert (_recordSet.get () != 0);
	Assert (_recordSet->GetTableId () == tableId);
	int count = _recordSet->RowCount ();
	rows.reserve (count);
	for (int i = 0; i < count; i++)
		rows.push_back (i);
}

void CmdLineSelection::GetSelectedRows (std::vector<unsigned> & rows, Table::Id tableId) const
{
	Assert (_recordSet.get () != 0);
	Assert (_recordSet->GetTableId () == tableId);
	int count = _recordSet->RowCount ();
	for (int i = 0; i < count; i++)
	{
		GlobalId id = _recordSet->GetGlobalId (i);
		if (id != gidInvalid)
		{
			// Use unique id
			rows.push_back (i);
		}
		else
		{
			// Use unique name
			GlobalId id = _recordSet->GetParentId (i);
			char const * name = _recordSet->GetName (i);
			UniqueName uname (id, name);
			UnameSet::const_iterator iter = _nameSet.find (uname);
			if (iter != _nameSet.end ())
			{
				rows.push_back (i);
			}
		}
	}
}
