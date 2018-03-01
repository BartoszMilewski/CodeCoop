//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "SelectIter.h"
#include "RecordSet.h"
#include "SelectionMan.h"
#include "UniqueName.h"
#include "Bookmark.h"

WindowSeq::WindowSeq (SelectionManager * selMan, Table::Id tableId) 
    : _cur (0),
      _recordSet (*selMan->GetRecordSet (tableId)) 
{}

WindowSeq::WindowSeq (SelectionManager * selMan) 
    : _cur (0),
      _recordSet (*selMan->GetCurRecordSet()) 
{}

bool WindowSeq::IsIncluded (GlobalId gid) const
{
	for (unsigned int i = 0; i < _rows.size (); i++)
	{
		if (_recordSet.GetGlobalId (_rows [i]) == gid)
			return true;
	}
	return false;
}

GlobalId WindowSeq::GetGlobalId () const 
{ 
	return _recordSet.GetGlobalId (_rows [_cur]); 
}

FileType WindowSeq::GetType () const 
{ 
	return _recordSet.GetType (_rows [_cur]); 
}

unsigned long WindowSeq::GetState () const 
{ 
	return _recordSet.GetState (_rows [_cur]); 
}

char const * WindowSeq::GetName () const 
{ 
	return _recordSet.GetName (_rows [_cur]); 
}

void WindowSeq::GetUniqueName (UniqueName & uname) const
{
    int row = _rows [_cur];
    GlobalId gidParent = _recordSet.GetParentId (row);
    char const * name = _recordSet.GetName (row);
	uname.Init (gidParent, name);
}

AllSeq::AllSeq (SelectionManager * selMan, Table::Id tableId)
    : WindowSeq (selMan, tableId)
{
	selMan->GetRows (_rows, tableId);
}

SelectionSeq::SelectionSeq (SelectionManager * selMan, Table::Id tableId)
    : WindowSeq (selMan, tableId)
{
	selMan->GetSelectedRows (_rows, tableId);
}

SelectionSeq::SelectionSeq (SelectionManager * selMan)
    : WindowSeq (selMan)
{
	selMan->GetSelectedRows(_rows);
}

SelectionSeq::SelectionSeq (SelectionManager * selMan, Table::Id tableId, std::vector<std::string> const & names)
    : WindowSeq (selMan, tableId)
{
	// Select rows from the record set by file name
	for (std::vector<std::string>::const_iterator iter = names.begin (); iter != names.end (); ++iter)
	{
		std::string const & fileName = *iter;
		Bookmark mark (gidInvalid, fileName);
		int row = _recordSet.GetRow (mark);
		if (row != 1)
			_rows.push_back (row);
	}
}

void SelectionSeq::ExpandRange ()
{
	if (_rows.size () < 3)
	{
		int startRow = 0;
		int stopRow = 0; 
		if (_rows.size () == 1)
		{
			stopRow = _rows [0];
		}
		else
		{
			Assert (_rows.size () == 2);
			if (_rows [0] < _rows [1])
			{
				startRow = _rows [0];
				stopRow = _rows [1]; 
			}
			else
			{
				startRow = _rows [1];
				stopRow = _rows [0]; 
			}
		}
		_rows.clear ();
		for (int i = startRow; i <= stopRow; ++i)
		{
			_rows.push_back (i);
		}
	}
}
