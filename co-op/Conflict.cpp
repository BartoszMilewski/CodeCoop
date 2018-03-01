//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "Conflict.h"
#include "Lineage.h"
#include "ScriptHeader.h"
#include "Model.h"
#include "DataBase.h"
#include "History.h"
#include "Workspace.h"

#include <File/Path.h>
#include <Ex/WinEx.h>
#include <Ctrl/ProgressMeter.h>
#include <Dbg/Out.h>

ConflictDetector::ConflictDetector (DataBase const & dataBase, History::Db const & history, ScriptHeader const & incomingHdr)
	: _myScriptsAreRejected (false),
	  _myLocalEditsInMergeConflict (false),
	  _isAutoMerge (true),
	  _conflictVersion (incomingHdr.GetComment ()),
	  _history (history),
	  _dataBase (dataBase)
{
	history.GetToBeRejectedScripts (_conflictRange);
	if (IsConflict ())
	{
		Progress::Meter dummyMeter;
		_selection.reset (new Workspace::HistoryRangeSelection (_history,
																_conflictRange,
																false,	// Backward selection
																dummyMeter
															   ));
		dbg << "Script conflict selection" << std::endl;
		dbg << *_selection;

		// Check if:
		//    a. My scripts are rejected
		//    b. The files I'm editing localy were changed by the rejected scripts

		// Select files/folders that have to be left checked-out after conflict resolution.
		// File/folder has to be left checked out when:
		//    - file/folder was checked out before script conflict was detected
		// or
		//    - it is a folder that was changed by a rejected script 
		//      (even by a script that was created by some other
		//      project member) AND has a child that was left checked-out
		GidSet suspiciousParents;
		GidSet undoneItems;
		for (Workspace::Selection::Sequencer seq (*_selection); !seq.AtEnd (); seq.Advance ())
		{
			Workspace::Item const & item = seq.GetItem ();
			GlobalId gid = item.GetItemGid ();
			undoneItems.insert (gid);
			// Start with source parent -- undo returns item to its source parent
			Workspace::Item const * parentItem = 0;
			if (item.GetSourceParentItem () != 0)
				parentItem = item.GetSourceParentItem ();
			else if (item.GetTargetParentItem () != 0)
				parentItem = item.GetTargetParentItem ();

			if (parentItem != 0)
			{
				// Parent item included in the selection (will be undone with its contents).
				if (!parentItem->HasEffectiveTarget () || !parentItem->HasEffectiveSource ())
				{
					// After undo the parent item either will be removed from the project (!HasTarget)
					// or will be added to the project (!HasEffectiveSource). This makes it a suspicious parent.
					suspiciousParents.insert (parentItem->GetItemGid ());
				}
			}

			FileData const * fd = _dataBase.GetFileDataByGid (gid);
			if (fd->GetState ().IsRelevantIn (Area::Original))
			{
				// Is currently checked-out
				_toBeLeftCheckedOut.insert (gid);
				_myLocalEditsInMergeConflict = true;
			}
			else
			{
				// Find out if script created by this user changes this file/folder
				for (GidList::const_iterator iter = _conflictRange.begin (); iter !=  _conflictRange.end (); ++iter)
				{
					GlobalIdPack pack (*iter);
					if (pack.GetUserId () == _dataBase.GetMyId ())
					{
						// Script send by this user
						if (_history.IsFileChangedByScript (gid, *iter))
						{
							// File changed by rejected script send out by this user
							_myScriptsAreRejected = true;
							break;	// Continue with next file
						}
					}
				}
			}
		}

		if (suspiciousParents.empty ())
			return;
		
		GidList uncheckoutItems;
		std::set_difference (undoneItems.begin (), undoneItems.end (),
							 _toBeLeftCheckedOut.begin (), _toBeLeftCheckedOut.end (),
							 std::back_inserter (uncheckoutItems));

		for (GidList::const_iterator iter = uncheckoutItems.begin ();
			 iter != uncheckoutItems.end ();
			 ++iter)
		{
			GlobalId uncheckoutGid = *iter;
			if (suspiciousParents.find (uncheckoutGid) != suspiciousParents.end ())
			{
				// Unchecked out item is a suspicious parent.
				// Add parent to the to be left checkedout list.
				_toBeLeftCheckedOut.insert (uncheckoutGid);
			}
			else
			{
				// Check parent of the unchedout item
				Workspace::Item const & item = _selection->FindItem (uncheckoutGid);
				Workspace::Item const * parentItem = 0;
				if (item.GetSourceParentItem () != 0)
					parentItem = item.GetSourceParentItem ();
				else if (item.GetTargetParentItem () != 0)
					parentItem = item.GetTargetParentItem ();

				if (parentItem != 0)
				{
					// Parent item included in the selection (will be undone with its contents).
					GlobalId parentGid = parentItem->GetItemGid ();
					if (_toBeLeftCheckedOut.find (parentGid) != _toBeLeftCheckedOut.end () &&
						suspiciousParents.find (parentGid) != suspiciousParents.end ())
					{
						// Unchecked out item's parent is to be left checked out and it is a suspisious parent.
						// Don't uncheckout this item.
						_toBeLeftCheckedOut.insert (uncheckoutGid);
					}
				}
			}
		}
	}
}

ConflictDetector::FileIterator::FileIterator (ConflictDetector const & detector)
	: _toBeLeftCheckedOut (detector._toBeLeftCheckedOut)
{
	for (Workspace::Selection::Sequencer seq (*detector._selection); !seq.AtEnd (); seq.Advance ())
	{
		_files.push_back (seq.GetItem ().GetItemGid ());
	}
	_cur = _files.begin ();
	_end = _files.end ();
}

bool ConflictDetector::FileIterator::IsToBeLeftCheckedOut () const
{
	GidSet::const_iterator iter = _toBeLeftCheckedOut.find (GetGlobalId ());
	return iter != _toBeLeftCheckedOut.end ();
}
