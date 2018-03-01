//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------

#include "precompiled.h"
#include "RecordSet.h"
#include "Image.h"
#include "SelectIter.h"
#include "TablePreferences.h"
#include "ColumnInfo.h"

unsigned int RecordSet::RowCount () const
{
	return _table.IsValid () ? _ids.size () : 0;
}

void RecordSet::GetImage (unsigned int row, SelectState state, int & iImage, int & iOverlay) const
{
	iImage = imageWait;
	iOverlay = overlayNone;
}

Bookmark RecordSet::GetBookmark (unsigned int row) const
{
	Bookmark bookmark (_ids [row], _names [row]);
	return bookmark;
}

unsigned int RecordSet::GetRow (Bookmark const & selection) const
{
	if (selection.IsValidGid ())
	{
		for (unsigned int i = 0; i < _ids.size (); ++i)
		{
			if (selection.GetGlobalId () == _ids [i])
				return i;
		}
	}
	else if (selection.IsNameValid ())
	{
		for (unsigned int i = 0; i < _ids.size (); ++i)
		{
			if (selection.GetName () == _names [i])
				return i;
		}
	}
	// Really, not found in the record set
	return Table::rowInvalid;
}

bool RecordSet::IsEmpty () const
{
	return IsValid () ? RowCount () == 0 : true;
}

DegreeOfInterest RecordSet::HowInteresting () const
{
	return IsEmpty () ? NotInteresting : VeryInteresting;
}

bool RecordSet::IsValid () const
{
	return _isValid && _table.IsValid ();
}

RowChange RecordSet::TranslateNotification (TableChange const & change)
{
	unsigned int row = change.IsAdd () ? Table::rowInvalid : GetRow (change);
	RowChange translated (change, row);
	return translated;
}

void RecordSet::StartUpdate ()
{
	_change.clear ();
	_startRowCount = RowCount ();
}

void RecordSet::AbortUpdate ()
{
	if (_change.size () != 0)
	{
		_change.clear ();
		if (_startRowCount < RowCount ())
			PopRows (_startRowCount);
	}
}

void RecordSet::CommitUpdate (bool delayRefresh)
{
	// For most record sets it is sufficient to update all observer items
	// Browse Window, which is an observer of this record set, will invalidate itself
	// and that means requesting from the table provider a new record set that will
	// replace this one.
	Assert (_table.IsObserver (this));
	if (!_isValid)	//	just check our own state (and not the table's)
	{
		//	this is a stale record set, so there's nothing to do
		Assert (_change.size () == 0);
		return;
	}

	if (_change.size () != 0)
	{
		//	RecordSet doesn't process individual changes - we just invalidate
		//	and force a refresh
		Invalidate ();

		//	Our observer will refresh, eventually
		Notify ();
	}
}

void RecordSet::ExternalUpdate (char const * topic)
{
	// Update all observer items as a result of external event reported to us
	NotifyIfNecessary (topic);
}

GlobalId RecordSet::GetGlobalId (unsigned int row) const
{
	Assert (row < _ids.size ());
	return _ids [row];
}

char const * RecordSet::GetName (unsigned int row) const
{
	Assert (row < _ids.size ());
	return _names [row].c_str ();
}

int RecordSet::CmpNames (unsigned int row1, unsigned int row2) const
{
	Assert (row1 < _names.size ());
	Assert (row2 < _names.size ());
	return FileNameCompare (_names [row1], _names [row2]);
}

int RecordSet::CmpStates (unsigned int row1, unsigned int row2) const
{
	Assert (row1 < _states.size ());
	Assert (row1 < _states.size ());
	return NocaseCompare (_states [row1], _states [row2]);
}

int RecordSet::CmpGids (unsigned int row1, unsigned int row2) const
{
	Assert (row1 < _ids.size ());
	Assert (row2 < _ids.size ());
	return _ids [row1] - _ids [row2];
}

void RecordSet::PushRow (GlobalId gid, std::string const & name)
{
	_ids.push_back (gid);
	_names.push_back (name.c_str ());
	_states.push_back ("");
}

void RecordSet::PopRows (int oldSize)
{
	_ids.resize (oldSize);
	_names.resize (oldSize);
	_states.resize (oldSize);
}

void RecordSet::DumpColHeaders (std::ostream & out, char fieldSeparator) const
{
	int cCol = ColCount ();
	int lastCol = cCol - 1;
	for (int i = 0; i < cCol; i++)
	{
		std::string const & name = _columns [i];
		out << name;
		if (i < lastCol)
			out << fieldSeparator;
	}
	out << std::endl;
}

void RecordSet::DumpRow (std::ostream & out, unsigned int row, char fieldSeparator) const
{
	Assert (_table.IsValid ());
	int cCol = ColCount ();
	int lastCol = cCol - 1;
	for (int i = 0; i < cCol; i++)
	{
		DumpField (out, row, i);
		if (i < lastCol)
			out << fieldSeparator;
	}
	out << std::endl;
}

void RecordSet::InitColumnHeaders (Column::Info const * columnInfo)
{
	for (unsigned int colNo = 0; strcmp(columnInfo [colNo].heading, "") != 0; ++colNo)
	{
		_columns.push_back (columnInfo [colNo].heading);
	}
}
