//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "TableBrowser.h"
#include "CoopMsg.h"
#include "AppInfo.h"
#include "UniqueName.h"
#include "Predicate.h"
#include "GidPredicate.h"
#include "Image.h"
#include "Global.h"
#include "FeedbackMan.h"
#include "HistoryScriptState.h"
#include "HistoryRange.h"

#include <Win/Message.h>
#include <StringOp.h>

#if !defined (NDEBUG)
std::ostream & operator<<(std::ostream & os, ChangeKind kind)
{
	switch (kind)
	{
	case changeEdit:
		os << "edit";
		break;
	case changeAdd:
		os << "add";
		break;
	case changeRemove:
		os << "remove";
		break;
	case changeAll:
		os << "all";
		break;
	default:
		os << "unknown";
		break;
	}
	return os;
}
#endif

class SortPredicate
{
public:
	SortPredicate (std::unique_ptr<RecordSet> const & recordSet, int sortCol, SortType sortOrder)
		: _recordSet (recordSet.get ()),
		  _sortOrder (sortOrder),
		  _sortCol (sortCol)
	{}

	bool operator ()(int rowX, int rowY);

private:
	int					_sortCol;
	SortType			_sortOrder;
	RecordSet const *	_recordSet;
};

bool SortPredicate::operator ()(int rowX, int rowY)
{
    // The CmpRows return value indicates the relation of rowX to rowY as follows.
    //
    // Return Value     Description
    //    < 0           rowX less than rowY
    //      0           rowX identical to rowY
    //    > 0           rowX greater than rowY
    int cmpResult = _recordSet->CmpRows (rowX, rowY, _sortCol);
    bool predicateValue = false;
    if (cmpResult != 0)
    {
        if (_sortOrder == sortAscend)
            predicateValue = cmpResult < 0;
        else
            predicateValue = cmpResult > 0;
	}
	return predicateValue;
}

//--------------------------
// TableBrowser::Selection
//--------------------------

void TableBrowser::Selection::RemoveItem (unsigned iItem)
{
	Assert (iItem < _state.size ());
	if (IsSelected (iItem))
		DeSelect (iItem);

	std::vector<SelectState>::iterator pos = _state.begin ();
	pos += iItem;
	_state.erase (pos);
}

void TableBrowser::Selection::Init (unsigned newSize)
{
	_state.resize (newSize);
	Clear ();
}

void TableBrowser::Selection::Clear ()
{
	_selCount = 0;
	_lastSelectedItem = InvalidValue;
	_firstRangeItem = InvalidValue;
	_lastRangeItem = InvalidValue;
	SelectState notSelected;
	std::fill (_state.begin (), _state.end (), notSelected);
}

void TableBrowser::Selection::ClearRange ()
{
	if (_firstRangeItem != InvalidValue)
	{
		for (unsigned item = _firstRangeItem; item <= _lastRangeItem; ++item)
			_state [item].SetRange (false);
		_firstRangeItem = InvalidValue;
		_lastRangeItem = InvalidValue;
	}
}

void TableBrowser::Selection::Select (unsigned iItem)
{
	Assert (iItem < _state.size ());
	Assert (_selCount <= _state.size ());
	Assert (!IsSelected (iItem));
	_state [iItem].Select ();
	_lastSelectedItem = iItem;
	_selCount++;
	Assert (_selCount <= _state.size ());
}

void TableBrowser::Selection::DeSelect (unsigned iItem)
{
	Assert (iItem < _state.size ());
	Assert (_selCount <= _state.size ());
	Assert (IsSelected (iItem));
	bool deselectedItemIsInRange = _state [iItem].IsRange ();
	_state [iItem].DeSelect ();
	_selCount--;
	Assert (_selCount <= _state.size ());
	if (_lastSelectedItem == iItem)
	{
		_lastSelectedItem = InvalidValue;
		if (_selCount > 0 && deselectedItemIsInRange)
		{
			unsigned nextItem = iItem + 1;
			unsigned prevItem = iItem - 1;
			if (nextItem < _state.size () && _state [nextItem].IsRange ())
				_lastSelectedItem = nextItem;
			else if (prevItem != InvalidValue && _state [prevItem].IsRange ())
				_lastSelectedItem = prevItem;
		}
	}
}

void TableBrowser::Selection::SetRange (unsigned iItem, bool flag)
{
	Assert (iItem < _state.size ());
	_state [iItem].SetRange (flag);
	if (_firstRangeItem == InvalidValue)
	{
		Assert (_lastRangeItem == InvalidValue);
		_firstRangeItem = iItem;
		_lastRangeItem = iItem;
	}
	else if (iItem < _firstRangeItem)
	{
		_firstRangeItem = iItem;
	}
	else if (iItem > _lastRangeItem)
	{
		_lastRangeItem = iItem;
	}
	Assert (_firstRangeItem <= iItem && iItem <= _lastRangeItem);
}

unsigned TableBrowser::Selection::FindFirstSelected (unsigned iItemStart) const
{
	std::vector<SelectState>::const_iterator iter =
		std::find_if (_state.begin () + iItemStart, _state.end (), IsSelectedItem ());
	if (iter != _state.end ())
		return iter - _state.begin ();
	else
		return InvalidValue;
}

//--------------------------
// TableBrowser::ViewRefresher
//--------------------------

void TableBrowser::ViewRefresher::UpdateView ()
{
	Assert (_browser._recordSet.get () != 0);
	// When record set name change rebuild view (create new column layout)
	if (!IsCaseEqual (Table::GetName (_browser._recordSet->GetTableId ()), _oldRecordSetName))
		_browser.RebuildView ();

	_browser.RepopulateView ();
	// Restore selection and position
	_browser.RestoreState ();
}

//--------------------------
// TableBrowser::ViewMemento
//--------------------------

TableBrowser::ViewMemento::ViewMemento (std::string const & visibleName)
{
	Bookmark visibleItem (visibleName);
	_scrollBookmarks.push_back (visibleItem);
	_selectedRows.push_back (visibleItem);
	_lastSelectedItem.reset (new Bookmark (visibleItem));
}

TableBrowser::ViewMemento::ViewMemento (TableBrowser const & browser)
{
	dbg << "--> TableBrowser::ViewMemento::ViewMemento - " << Table::GetName (browser.GetTableId ()) << std::endl;;
	browser.GetScrollBookmarks (_scrollBookmarks);
	browser.GetSelectionBookmarks (_selectedRows, _lastSelectedItem);
	dbg << "<-- TableBrowser::ViewMemento::ViewMemento - " << Table::GetName (browser.GetTableId ()) << std::endl;;
}

// Returns true when restored selection is not empty
bool TableBrowser::ViewMemento::RestoreState (TableBrowser & browser)
{
	dbg << "--> TableBrowser::ViewMemento::RestoreState - " << Table::GetName (browser.GetTableId ()) << std::endl;;
	bool selectionRestored = browser.SetSelectionBookmarks (_selectedRows, _lastSelectedItem.get ());
	browser.SetScrollBookmarks (_scrollBookmarks);
	dbg << "<-- TableBrowser::ViewMemento::RestoreState - " << Table::GetName (browser.GetTableId ()) << std::endl;;
	return selectionRestored;
}

//--------------------------
// TableBrowser
//--------------------------

TableBrowser::TableBrowser (TableProvider & tableProv,
							Table::Id tableId,
							::Preferences::Storage const & storage,
							std::string const & paneName,
							Column::Info const * columnInfo,
							TableViewer & view)
	: WidgetBrowser (tableProv, tableId),
	  _preferences (storage, paneName, columnInfo),
	  _tableView (view),
	  _isVisible (false),
	  _viewNeedsUpdate (true)
{
	_restriction.SetExtensionFilter (_preferences.GetFilter ());
	NamedBool const & options =  _preferences.GetOptions ();
	// In Files pane, if extension filter is empty, the filter should default to HideNonProject
	if (paneName == "Files" && _preferences.GetFilter ().empty () && !options.IsOn ("HideNonProject"))
		_restriction.Set ("HideNonProject");
	std::copy (options.begin (), options.end (), std::inserter (_restriction, _restriction.end ()));
}

bool TableBrowser::IsSelectionAtEnd () const
{
	if (_recordSet.get () == 0 || !_recordSet->IsValid ())
		return false;

	Assert (_sortVector.size () == _recordSet->RowCount ());
	int count = _sortVector.size ();
	for (int i = 0; i < count; i++)
	{
		if (IsSelected (i))
		{
			unsigned int row = _sortVector [i];
			return row == _recordSet->RowCount () - 1;
		}
	}
	return false;
}

void TableBrowser::SelectIf (std::function<bool(long, long)> predicate)
{
	Assert (_isVisible);
	Assert (_recordSet.get () != 0);
	if (_recordSet->IsValid ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		int count = _sortVector.size ();
		for (int i = 0; i < count; i++)
		{
			int row = _sortVector [i];
			if (predicate (_recordSet->GetState (row), _recordSet->GetType (row)))
			{
				int item = RowToViewIndex (row);
				_tableView.Select (item);
				_tableView.SetFocus (item);
				return;
			}
		}
	}
}

int TableBrowser::SelectIf (NamePredicate const & predicate)
{
	Assert (_isVisible);
	Assert (_recordSet.get () != 0);
	if (_recordSet->IsValid ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		int count = _sortVector.size ();
		for (int i = 0; i < count; i++)
		{
			int row = _sortVector [i];
			if (predicate (_recordSet->GetName (row)))
			{
				int item = RowToViewIndex (row);
				_tableView.Select (item);
				return item;
			}
		}
	}
	return InvalidValue;
}

void TableBrowser::SelectIds (GidList const & ids)
{
	Assert (_isVisible);
	Assert (_recordSet.get () != 0);
	GidSet idSet (ids.begin (), ids.end ());
	if (_recordSet->IsValid ())
	{
		_tableView.DeSelectAll ();
		Assert (_sortVector.size () == _recordSet->RowCount ());
		int count = _sortVector.size ();
		for (int i = 0; i < count && !idSet.empty (); i++)
		{
			int row = _sortVector [i];
			GlobalId rowId = _recordSet->GetGlobalId (row);
			GidSet::iterator iter = idSet.find (rowId);
			if (iter != idSet.end ())
			{
				int item = RowToViewIndex (row);
				_tableView.Select (item);
				_tableView.PostEnsureVisible (item);
				idSet.erase (iter);
			}
		}
	}
}

void TableBrowser::SelectItems (std::vector<unsigned> const & items)
{
	Assert (_isVisible);
	Assert (_recordSet.get () != 0);
	if (_recordSet->IsValid ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		_tableView.DeSelectAll ();
		for (std::vector<unsigned>::const_iterator iter = items.begin ();
			 iter != items.end ();
			 ++iter)
		{
			unsigned item = *iter;
			_tableView.Select (item);
			_tableView.PostEnsureVisible (item);
		}
	}
}

bool TableBrowser::Show (FeedbackManager & feedback)
{
	dbg << "--> TableBrowser::Show - " << Table::GetName (_tableId) << std::endl;;
	Assert (!_isVisible);
	TableBrowser::ViewRefresher refresher (*this);
	std::string statusText;
	statusText += "Refreshing ";
	statusText += Table::GetName (_tableId);
	StatusTextSwitch textSwitch (feedback, statusText);
	RefreshRecordSetIfNecessary (false, &feedback);	// Not a notification
	refresher.UpdateView ();
	_isVisible = true;
	Assert (!_viewNeedsUpdate);
	dbg << "<-- TableBrowser::Show - " << Table::GetName (_tableId) << std::endl;
	return false;
}

void TableBrowser::Hide ()
{
	dbg << "--> TableBrowser::Hide - " << Table::GetName (_tableId) << std::endl;
	if (_isVisible)
	{
		Assert (!_viewNeedsUpdate);
		Assert (_recordSet.get () != 0);
		if (!_recordSet->IsDummy ())
		{
			// Save view preferences
			_preferences.SetFilter (_restriction.GetExtensionFilter ());
			// Remove volatile options before remembering them in the prefrences storage
			NamedBool savedOptions = _restriction;
			NamedBool::iterator iter = savedOptions.find ("RangeMerge");
			if (iter != savedOptions.end ())
				savedOptions.erase (iter);
			_preferences.SetOptions (savedOptions);
			// Remember column widths
			for (unsigned int colIdx = 0; colIdx < _tableView.GetColCount (); ++colIdx)
			{
				unsigned int pixelWidth = _tableView.GetColumnWidth (colIdx);
				_preferences.SetColumnWidth (colIdx, pixelWidth);
			}
			// Save selection and position
			SaveState ();
		}
		_isVisible = false;
	}
	Assert (!_isVisible);
	dbg << "<-- TableBrowser::Hide - " << Table::GetName (_tableId) << std::endl;
}

void TableBrowser::OnFocus ()
{
	dbg << "--> TableBrowser::OnFocus - " << Table::GetName (_tableId) << std::endl;
	if (_isVisible)
	{
		Assert (!_viewNeedsUpdate);
		_tableView.SetFocus ();
		if (_tableView.GetCount () != 0)
		{
			unsigned focusItem = (_selection.Count () != 0) ? _selection.GetLastSelectedItem () : 0;
			_tableView.SetFocus (focusItem);
			dbg << "     Focus item: " << std::dec << focusItem << std::endl;
			if (_selection.Count () == 0)
			{
				// When there is no selection then force displaying frame around
				// the focused item (setting focus in not enough).
				dbg << "     Forcing frame" << std::endl;
				Assert (focusItem == 0);
				_tableView.SendMsg (Win::Message (WM_KEYDOWN, VK_UP));
				_tableView.SendMsg (Win::Message (WM_KEYUP, VK_UP));
			}
		}
	}
	else
	{
		dbg << "     Not visible yet" << std::endl;
	}
	dbg << "<-- TableBrowser::OnFocus - " << Table::GetName (_tableId) << std::endl;
}

void TableBrowser::Invalidate ()
{
	dbg << "--> TableBrowser::Invalidate - " << Table::GetName (_tableId) << std::endl;;
	if (!_isVisible)
	{
		// Browser window is not visible -- just invalidate the record set
		dbg << "    Not visible" << std::endl;
		if (_recordSet.get () != 0)
		{
			_recordSet->Invalidate ();
		}
		else
		{
			dbg << "    No record set" << std::endl;
		}
	}
	else
	{
		// Already showing some record set
		dbg << "    Visible already showing some record set" << std::endl;
		Assert (_recordSet.get () != 0);
		TableBrowser::ViewRefresher refresher (*this);
		if (RefreshRecordSetIfNecessary (true))	// Notification
			refresher.UpdateView ();
	}
	dbg << "<-- TableBrowser::Invalidate - " << Table::GetName (_tableId) << std::endl;;
}

bool TableBrowser::RefreshRecordSetIfNecessary (bool isNotification, FeedbackManager * feedback)
{
	// Change record set when:
	//	1. There is no current record set.
	//	2. Current record set is invalid.
	//	3. Current record set is from different table then browser expected table
	//	   (for example file view browser displays empty table record set while
	//	   not-in-project state).
	//	4. The new record set is different from the current one (they both have to
	//	   came from the same table).
	dbg << "--> TableBrowser::RefreshRecordSetIfNecessary - " << Table::GetName (_tableId) << std::endl;;
	if (feedback != 0)
	{
		feedback->SetRange (0, 5, 1);
		feedback->StepIt ();
	}
	if (_recordSet.get () == 0)
	{
		// No current record set
		_recordSet = _tableProv.Query (_tableId, _restriction);
		OnFirstRefresh ();
	}
	else if (!_recordSet->IsValid ())
	{
		// Current record set was invalidated when we didn't display it 
		if (_memento.get () == 0)
			SaveState ();	// Save state of the current record set if we didn't record it earlier
		_recordSet->Detach (this);
		_recordSet = _tableProv.Query (_tableId, _restriction);
	}
	else if (isNotification)
	{
		// Current record set is valid and we are notified about change.
		// Compare record set captions and rows to find out if really something changed.
		std::unique_ptr<RecordSet> newRecordSet (_tableProv.Query (_tableId, _restriction));
		std::string newCaption = _tableProv.QueryCaption (_tableId, _restriction);
		if (newCaption == _caption && newRecordSet->IsEqual (_recordSet.get ()))
		{
			dbg << "<-- TableBrowser::RefreshRecordSetIfNecessary -- nothing have changed - " << Table::GetName (_tableId) << std::endl;
			return false;
		}
		// Save selection and position in the old record set
		SaveState ();
		_recordSet->Detach (this);
		_recordSet = std::move(newRecordSet);
	}
	else
	{
		dbg << "<-- TableBrowser::RefreshRecordSetIfNecessary -- nothing have changed - " << Table::GetName (_tableId) << std::endl;
		return false;
	}

	Assert (_recordSet.get () != 0);
	_caption = _tableProv.QueryCaption (_tableId, _restriction);
	if (feedback != 0)
		feedback->StepIt ();
	// Make this window the observer of record set
	_recordSet->Attach (this);
	// Refresh record set preferences
	unsigned int sortColumnIdx;
	SortType sortOrder;
	_preferences.GetSortInfo (sortColumnIdx, sortOrder);
	_restriction.SetSort (sortColumnIdx, sortOrder);
	unsigned int rowCount = _recordSet->RowCount ();
	dbg << "    New record set row count: " << std::dec << rowCount << std::endl;
	// Make sure that all book-keeping vectors have the right size
	_selection.Init (rowCount);
	InitSort ();
	if (feedback != 0)
		feedback->StepIt ();
	if (rowCount > 1)
		Sort ();
	else
		_memento.reset (0);	// Clear previous state saved by SaveState

	if (feedback != 0)
		feedback->StepIt ();
	Assert (rowCount == _sortVector.size ());
	_viewNeedsUpdate = true;
	if (feedback != 0)
		feedback->StepIt ();
	Notify ("tree");
	dbg << "<-- TableBrowser::RefreshRecordSetIfNecessary - new record set - " << Table::GetName (_tableId) << std::endl;
	return true;
}

void TableBrowser::RebuildView ()
{
	dbg << "--> TableBrowser::RebuildView - " << Table::GetName (_tableId) << std::endl;;
    Assert (_recordSet.get () != 0);
	_tableView.ClearAll ();
	// Add columns
	if (_recordSet->IsDummy ())
	{
		// We've got empty record set
		Assert ( _recordSet->ColCount () == 1);
		_tableView.AddColumn (_preferences.GetEmptyTableColumnWidth (),
							   "",
							   Win::Report::Left,
							   false);	// No images
	}
	else
	{
		int cCol = _recordSet->ColCount ();
		for (int i = 0; i < cCol; i++)
		{
			std::string const & name = _preferences.GetColumnHeading (i);
			Win::Report::ColAlignment colAlign = _preferences.GetColumnAlignment (i);
			unsigned int columnWidth = _preferences.GetColumnWidth (name);
			_tableView.AddColumn (columnWidth, name, colAlign, true);	// Has images
		}

		// Place sort marker in the sort column header
		unsigned int sortCol;
		SortType order = _restriction.GetSortCol (sortCol);
		if (order != sortNone)
		{
			Assert (order == sortAscend || order == sortDescend);
			int iImage = order == sortAscend ? 1 : 0;
			_tableView.SetColumnImage (sortCol, iImage);
		}
	}
	dbg << "<-- TableBrowser::RebuildView - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::RepopulateView ()
{
	dbg << "--> TableBrowser::RepopulateView - " << Table::GetName (_tableId) << std::endl;;
	unsigned itemCount = _recordSet->RowCount ();
	if (itemCount > 0)
	{
		_tableView.Reserve (itemCount);
		_tableView.RedrawItems (0, itemCount - 1);
	}
	else
	{
		_tableView.ClearRows ();
	}
	_viewNeedsUpdate = false;
	Assert (_tableView.GetCount () == itemCount);
	dbg << "<-- TableBrowser::RepopulateView - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::SaveState ()
{
	dbg << "--> TableBrowser::SaveState - " << Table::GetName (_tableId) << std::endl;
	// Remember bottom visible position
	Assert (_recordSet.get () != 0);
	if (_recordSet->RowCount () == 0)
	{
		_memento.reset (0);
		dbg << "<-- TableBrowser::SaveState - " << Table::GetName (_tableId) << " -- empty record set" << std::endl;;
		return;	// Nothing to save
	}

	_memento.reset (new TableBrowser::ViewMemento (*this));
	_tableView.DeSelectAll ();
	dbg << "<-- TableBrowser::SaveState - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::RestoreState ()
{
	dbg << "--> TableBrowser::RestoreState - " << Table::GetName (_tableId) << std::endl;
	Assert (!_viewNeedsUpdate);
	Assert (_recordSet.get () != 0);
	bool restoredSelectionIsEmpty = true;
	if (_memento.get () != 0)
	{
		Assert (_tableView.GetSelectedCount () == 0);
		restoredSelectionIsEmpty = !_memento->RestoreState (*this);
		_memento.reset (0);
	}

	if (restoredSelectionIsEmpty && _recordSet->IsDefaultSelection ())
	{
		// Restored selection is empty but the record set has a default selection.
		// Select it in the view.
		unsigned row = _recordSet->GetDefaultSelectionRow ();
		std::vector<unsigned>::const_iterator iter =
			std::find (_sortVector.begin (), _sortVector.end (), row);
		Assert (iter != _sortVector.end ());
		unsigned item = iter - _sortVector.begin ();
		// Selection state will be updated automatically via SelChanged
		_tableView.Select (item);
	}

	Assert (_tableView.GetSelectedCount () == _selection.Count ());
	dbg << "<-- TableBrowser::RestoreState - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::Clear (bool forGood)
{ 
	dbg << "--> TableBrowser::Clear - " << Table::GetName (_tableId) << (forGood ? " (for good)" : "") << std::endl;
	_selection.Clear ();
	_restriction.ClearFilters ();
	_caption.erase ();
	if (_recordSet.get () != 0)
	{
		if (forGood)
		{
			// Clearing for good removes record set thus makes
			// the browser data invisible
			_recordSet.reset ();
			_isVisible = false;
		}
		else
		{
			// Keep old record set, so internal notification flow
			// is not broken, but invalidate it
			_recordSet->Invalidate ();
		}
	}
	// Remove all rows, but leave column headers
	_tableView.ClearRows ();
    _sortVector.clear ();
	_memento.reset (0);
	dbg << "<-- TableBrowser::Clear - " << Table::GetName (_tableId) << std::endl;;
}

//
// View notifications
//

void TableBrowser::CopyField (unsigned iItem, unsigned int col, char * buf, unsigned int bufLen) const
{
	Assert (_recordSet.get () != 0);
	if (iItem < _recordSet->RowCount () && col < _recordSet->ColCount ())
	{
		unsigned int requestedRow = GetRow (iItem);
		Assert (requestedRow < _recordSet->RowCount ());
		_recordSet->CopyStringField (requestedRow, col, bufLen, buf);
	}
	else
	{
		Assert (bufLen > 0);
		buf [0] = '\0';
	}
}

void TableBrowser::GetImage (unsigned iItem, int & iImage, int & iOverlay) const
{
	Assert (_recordSet.get () != 0);
	if (iItem < _recordSet->RowCount ())
	{
		unsigned int requestedRow = GetRow (iItem);
		Assert (requestedRow < _recordSet->RowCount ());
		// Revisit: _state [iItem] is never used by any record set !
		_recordSet->GetImage (requestedRow, _selection.State (iItem), iImage, iOverlay);
	}
	else
	{
		iImage = 0;
		iOverlay = 0;
	}
}

void TableBrowser::SelChanged (unsigned iItem, bool wasSelected, bool isSelected)
{
	Assert (_recordSet.get () != 0);
	if (!_recordSet->IsValid ())
		return;

	if (!wasSelected && isSelected)
        Select (iItem);
    else if (wasSelected && !isSelected)
        DeSelect (iItem);

	Assert (_tableView.GetSelectedCount () == _selection.Count ());
}

void TableBrowser::Select (unsigned iItem)
{
	_selection.Select (iItem);
	dbg << "    TableBrowser::Select - " << Table::GetName (_tableId) << "; item: " << std::dec << iItem << "; selected count: " << _selection.Count () << std::endl;
}

void TableBrowser::DeSelect (unsigned iItem)
{
	_selection.DeSelect (iItem);
	dbg << "    TableBrowser::DeSelect - " << Table::GetName (_tableId) << "; item: " << std::dec << iItem << "; selected count: " << _selection.Count () << std::endl;
}

void TableBrowser::InitSort ()
{
	Assert (_recordSet.get () != 0);
	// Init sort vector
	_sortVector.clear ();
	unsigned int rowCount = _recordSet->RowCount ();
	for (unsigned int i = 0; i < rowCount; i++)
	{
		_sortVector.push_back (i);
	}
}

bool TableBrowser::Sort ()
{
	dbg << "--> TableBrowser::Sort - " << Table::GetName (_tableId) << std::endl;;
	unsigned int primarySortCol;
    SortType primarySortOrder = _restriction.GetSortCol (primarySortCol);
	dbg << "   sort by column: " << primarySortCol << std::endl;
    if (primarySortOrder != sortNone && _sortVector.size () > 1)
    {
		Assert (primarySortCol != Table::colInvalid);
		Assert (_sortVector.size () == _recordSet->RowCount ());
		Assert (primarySortOrder == sortAscend || primarySortOrder == sortDescend);
		SortType secondarySortOrder = _preferences.GetSecondarySortOrder ();
		if (secondarySortOrder != sortNone)
		{
			Assert (secondarySortOrder == sortAscend || secondarySortOrder == sortDescend);
			unsigned int secondarySortCol = _preferences.GetSecondarySortColumnIdx ();
			Assert (secondarySortCol != Table::colInvalid);
			dbg << "   Sort" << std::endl;
			std::sort (_sortVector.begin (),
					   _sortVector.end (),
					   SortPredicate (_recordSet, secondarySortCol, secondarySortOrder));
		}
		dbg << "   Stable sort" << std::endl;
		std::stable_sort (_sortVector.begin (),
						  _sortVector.end (),
						  SortPredicate (_recordSet, primarySortCol, primarySortOrder));
		dbg << "<-- TableBrowser::Sort -- sorted - " << Table::GetName (_tableId) << std::endl;;
		return true;
	}
	dbg << "<-- TableBrowser::Sort -- not sorted - " << Table::GetName (_tableId) << std::endl;;
    return false;
}

void TableBrowser::Resort (unsigned int newSortCol)
{
	dbg << "--> TableBrowser::Resort - " << Table::GetName (_tableId) << std::endl;;
	Assert (_recordSet.get () != 0);
	unsigned int prevSortCol = _preferences.GetSortColumnIdx ();
	SortType prevSortOrder = _preferences.GetSortOrder ();

	if (newSortCol == prevSortCol)
	{
		// User wants to change the sort order of the current sort column
		_preferences.ChangeCurrentSortOrder ();
	}
	else
	{
		// User selected new sort column - use column default sort order
		_preferences.SetSortColumn (newSortCol);
	}
	SortType newSortOrder = _preferences.GetSortOrder ();
	_restriction.SetSort (newSortCol, newSortOrder);

	if (newSortOrder != sortNone && (newSortOrder != prevSortOrder || newSortCol != prevSortCol))
	{
		// Sort order or sort column change - update column marker
		_tableView.RemoveColumnImage (prevSortCol);
		Assert (newSortOrder == sortAscend || newSortOrder == sortDescend);
		int iImage = (newSortOrder == sortAscend ? 1 : 0);
		_tableView.SetColumnImage (newSortCol, iImage);

		if (!IsEmpty ())
		{
			SaveState ();
			Sort ();
			RepopulateView ();
			RestoreState ();
		}
	}
	_tableView.SetFocus ();
	dbg << "<-- TableBrowser::Resort - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::InPlaceEdit (unsigned row)
{
    _tableView.InPlaceEdit (RowToViewIndex (row));
}

void TableBrowser::BeginNewItemEdit ()
{
	// Add to the record set row used for
	// list view label editing
    _recordSet->BeginNewItemEdit ();
	unsigned newRow = _recordSet->RowCount () - 1;
	_selection.AppendItem ();
	_sortVector.push_back (newRow);
	_tableView.AppendItem ();
	Assert (_sortVector.size () == _recordSet->RowCount ());

	// Make added row visible in the list view control
    unsigned idx = RowToViewIndex (newRow);
	Assert (idx != InvalidValue);
    _tableView.EnsureVisible (idx);
	// Select label edit item and place focus there
    _tableView.Select (idx);
    _tableView.SetFocus (idx);
	// Place focus on the list view control, so user can
	// type in new name
	_tableView.SetFocus ();
	// Send to the main window OnBeginLableEdit
    _tableView.InPlaceEdit (idx);
}

void TableBrowser::AbortNewItemEdit ()
{
	// Remove from the list view label editing item
    unsigned row = _recordSet->RowCount () - 1;
	std::vector<unsigned>::iterator iter =
		std::find (_sortVector.begin (), _sortVector.end (), row);
	Assert (iter != _sortVector.end ());
	unsigned idx = iter - _sortVector.begin ();
	_tableView.DeSelect (idx);
	// Remove row from the browser bookkeeping data
	_tableView.RemoveItem (idx);
	_selection.RemoveItem (idx);
	// Remove the row from the record set
    _recordSet->AbortNewItemEdit ();
	_sortVector.erase (iter);
	Assert (_sortVector.size () == _recordSet->RowCount ());
	// Repaint the list view control
	Invalidate ();
}

void TableBrowser::ScrollToItem (unsigned iItem)
{
	Assert (_isVisible);
	_tableView.EnsureVisible (iItem);
}

// Find screen position corresponding to a given RecordSet row

unsigned TableBrowser::RowToViewIndex (unsigned row) const
{
	std::vector<unsigned>::const_iterator iter =
		std::find (_sortVector.begin (), _sortVector.end (), row);
	if (iter != _sortVector.end ())
		return iter - _sortVector.begin ();

	return InvalidValue;
}

void TableBrowser::GetScrollBookmarks (std::vector<Bookmark> & bookmarks) const
{
	// Save scroll bookmarks
	Assert (_recordSet.get () != 0);
	unsigned rowCount = _recordSet->RowCount ();
	Assert (rowCount != 0);
	Assert (rowCount == _tableView.GetCount ());
	unsigned topVisibleItem = _tableView.GetTopIndex ();
	Assert (topVisibleItem < rowCount);
	if (topVisibleItem >= rowCount)
		topVisibleItem = 0;
	unsigned bottomVisibleItem = topVisibleItem + _tableView.GetCountPerPage () - 1;
	if (bottomVisibleItem >= rowCount)
		bottomVisibleItem = rowCount - 1;
	unsigned row = _sortVector [bottomVisibleItem];
	dbg << "      Bottom visible row: " << std::dec << row << std::endl;
	Bookmark bottomBookMark (_recordSet->GetBookmark (row));
	row = _sortVector [topVisibleItem];
	dbg << "      Top visible row: " << std::dec << row << std::endl;
	Assert (row < rowCount);
	Bookmark topBookMark (_recordSet->GetBookmark (row));
	if (IsUpsideDown ())
	{
		dbg << "     Upside down" << std::endl;
		bookmarks.push_back (topBookMark);
		bookmarks.push_back (bottomBookMark);
	}
	else
	{
		bookmarks.push_back (bottomBookMark);
		bookmarks.push_back (topBookMark);
	}
	unsigned lastSelectedItem = _selection.GetLastSelectedItem ();
	if (lastSelectedItem != InvalidValue &&
		topVisibleItem <= lastSelectedItem &&
		lastSelectedItem <= bottomVisibleItem)
	{
		// Last selected item is visible - save its bookmark
		row = _sortVector [lastSelectedItem];
		dbg << "      Last selected row: " << std::dec << row << " is visible" << std::endl;
		bookmarks.push_back (_recordSet->GetBookmark (row));
	}
}

void TableBrowser::SetScrollBookmarks (std::vector<Bookmark> const & bookmarks)
{
	// Restore scroll position
	for (std::vector<Bookmark>::const_iterator iter = bookmarks.begin ();
		 iter != bookmarks.end ();
		 ++iter)
	{
		unsigned row = _recordSet->GetRow (*iter);
		if (row != Table::rowInvalid)
		{
			// Bookmarked row still present in the record set
			std::vector<unsigned>::const_iterator iter =
				std::find (_sortVector.begin (), _sortVector.end (), row);
			Assert (iter != _sortVector.end ());
			unsigned item = iter - _sortVector.begin ();
			dbg << "     Ensure visible item: " << std::dec << item << std::endl;
			_tableView.PostEnsureVisible (item);
		}
	}
}

void TableBrowser::GetSelectionBookmarks (std::vector<Bookmark> & bookmarks,
										  std::unique_ptr<Bookmark> & lastSelectionBookmark) const
{
	Assert (_recordSet.get () != 0);
	unsigned rowCount = _recordSet->RowCount ();
	Assert (rowCount != 0);
	Assert (rowCount == _tableView.GetCount ());
	unsigned row;
	// Save selection bookmarks
	if (_selection.Count () != 0)
	{
		for (unsigned i = 0; i < rowCount; ++i)
		{
			if (_selection.IsSelected (i))
			{
				row = _sortVector [i];
				dbg << "      Selected row: " << std::dec << row << std::endl;
				bookmarks.push_back (_recordSet->GetBookmark (row));
			}
		}
		unsigned lastSelectedItem = _selection.GetLastSelectedItem ();
		if (lastSelectedItem != InvalidValue)
		{
			row = _sortVector [lastSelectedItem];
			dbg << "      Last selected item: " << std::dec << lastSelectedItem << "; row: " << std::dec << row << std::endl;
			lastSelectionBookmark.reset (new Bookmark (_recordSet->GetBookmark (row)));
		}
	}
	Assert (_selection.Count () == bookmarks.size ());
}

// Returns true when selection restored
bool TableBrowser::SetSelectionBookmarks (std::vector<Bookmark> const & bookmarks,
										  Bookmark const * lastSelectionBookmark)
{
	// Restore selection bookmarks
	_selection.Clear ();
	bool selectionRestored = false;
	for (std::vector<Bookmark>::const_iterator iter = bookmarks.begin ();
		 iter != bookmarks.end ();
		 ++iter)
	{
		unsigned row = _recordSet->GetRow (*iter);
		if (row != Table::rowInvalid)
		{
			selectionRestored = true;
			std::vector<unsigned>::const_iterator iter =
				std::find (_sortVector.begin (), _sortVector.end (), row);
			Assert (iter != _sortVector.end ());
			unsigned item = iter - _sortVector.begin ();
			dbg << "     Selecting item: " << std::dec << item << std::endl;
			// Selection state will be updated automatically via SelChanged
			_tableView.Select (item);
		}
	}

	if (lastSelectionBookmark != 0)
	{
		// Set focus the the last selected item
		unsigned row = _recordSet->GetRow (*lastSelectionBookmark);
		if (row != Table::rowInvalid)
		{
			std::vector<unsigned>::const_iterator iter =
				std::find (_sortVector.begin (), _sortVector.end (), row);
			Assert (iter != _sortVector.end ());
			unsigned item = iter - _sortVector.begin ();
			_tableView.SetFocus (item);
			dbg << "     Focusing item: " << std::dec << item << std::endl;
		}
	}
	return selectionRestored;
}

void TableBrowser::SetRange (History::Range const & range)
{
	if (_recordSet.get () == 0 || !_recordSet->IsValid ())
		return;

	Assert (_sortVector.size () == _recordSet->RowCount ());
	// Clear old range
	unsigned firstRangeItem;
	unsigned lastRangeItem;
	_selection.GetRangeLimits (firstRangeItem, lastRangeItem);
	_selection.ClearRange ();
	if (firstRangeItem != InvalidValue && lastRangeItem != InvalidValue)
		_tableView.RedrawItems (firstRangeItem, lastRangeItem);

	if (range.Size () == 0)
		return;

	// Set new range
	unsigned rowCount = _sortVector.size ();
	for (unsigned rangeIdx = 0; rangeIdx < range.Size (); ++rangeIdx)
	{
		unsigned sortVectorIdx = 0;
		GlobalId rangeId = range [rangeIdx];
		// Find range id in the view
		while (sortVectorIdx < rowCount &&
			   _recordSet->GetGlobalId (_sortVector [sortVectorIdx]) != rangeId)
		{
			++sortVectorIdx;
		}
		Assert (sortVectorIdx == rowCount ||
				_recordSet->GetGlobalId (_sortVector [sortVectorIdx]) == rangeId);
		if (sortVectorIdx < rowCount)
			_selection.SetRange (sortVectorIdx, true);
	}
	_selection.GetRangeLimits (firstRangeItem, lastRangeItem);
	_tableView.RedrawItems (firstRangeItem, lastRangeItem);
}

bool TableBrowser::FindIfSome (std::function<bool(long, long)> predicate) const
{
	if (_recordSet.get () != 0 && _recordSet->IsValid () && !_recordSet->IsDummy ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		for (std::vector<unsigned>::const_iterator iter = _sortVector.begin ();
			 iter != _sortVector.end ();
			 ++iter)
		{
			unsigned row = *iter;
			if (predicate (_recordSet->GetState (row), _recordSet->GetType (row)))
				return true;
		}
	}
	return false;
}

bool TableBrowser::FindIfSelected (std::function<bool(long, long)> predicate) const
{
	if (_recordSet.get () != 0 && _recordSet->IsValid () && !_recordSet->IsDummy ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		int count = _sortVector.size ();
		for (int i = 0; i < count; i++)
		{
			if (IsSelected (i))
			{
				int row = _sortVector [i];
				if (predicate (_recordSet->GetState (row), _recordSet->GetType (row)))
					return true;
			}
		}
	}
	return false;
}

bool TableBrowser::FindIfSelected (GidPredicate const & predicate) const
{
	if (_recordSet.get () != 0 && _recordSet->IsValid () && !_recordSet->IsDummy ())
	{
		Assert (_sortVector.size () == _recordSet->RowCount ());
		int count = _sortVector.size ();
		for (int i = 0; i < count; i++)
		{
			if (IsSelected (i))
			{
				int row = _sortVector [i];
				if (predicate (_recordSet->GetGlobalId (row)))
					return true;
			}
		}
	}
	return false;
}

unsigned TableBrowser::FindSelection (unsigned iStart) const
{
	return _selection.FindFirstSelected (iStart);
}

void TableBrowser::GetSelectedRows (std::vector<unsigned> & rows) const
{
	unsigned count = _recordSet->RowCount ();
	for (unsigned int i = 0; i < count; i++)
	{
		if (_selection.IsSelected (i))
		{
			rows.push_back (GetRow (i));
		}
	}
	if (IsUpsideDown ())
		std::reverse (rows.begin (), rows.end ());
}

void TableBrowser::GetAllRows (std::vector<unsigned> & rows) const
{
	unsigned int count = _recordSet->RowCount ();
	for (unsigned int i = 0; i < count; i++)
	{
		rows.push_back (GetRow (i));
	}
}

void TableBrowser::UpdateAll ()
{
	dbg << "--> TableBrowser::UpdateAll - " << Table::GetName (_tableId) << std::endl;;
	// Avoid deadlock by not doing anything which might send a message

	// Revisit: Is there a better way to post a message?  We can't rely
	// on Notify because not all browse windows are observed...
	// Post a refresh message (to avoid sending messages)
	dbg << "    Posting UM_REFRESH_BROWSEWINDOW message to the application window" << std::endl;
	Win::UserMessage um (UM_REFRESH_BROWSEWINDOW);
	um.SetLParam (this);
	TheAppInfo.GetWindow ().PostMsg (um);

	// Notify table browser observers
	Notify ();
	dbg << "<-- TableBrowser::UpdateAll - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::Update (std::vector<RowChange> const & change)
{
	dbg << "--> TableBrowser::Update - " << Table::GetName (_tableId) << std::endl;;
	Assert (_recordSet.get () != 0);
	if (!_isVisible)
	{
		// This window is invisible -- nothing to do.
		// Record set already contains refreshed rows
		_viewNeedsUpdate = true;
		dbg << "<-- TableBrowser::Update -- invisible - " << Table::GetName (_tableId) << std::endl;;
		return;
	}

	// Update items of changed rows
	bool repaintRequired = false;
	typedef std::vector<RowChange>::const_iterator ChangeIter;
	dbg << "   Iterating over " << change.size() << " change(s)" << std::endl;
	for (ChangeIter iter = change.begin (); iter != change.end (); ++iter)
	{
		Assert (iter->IsEdit ());
		int row = iter->GetRow ();
		if (row == InvalidValue)
			continue;	// Row is not displayed in this browse window
		// Check if item sort order was changed
		if (NeedSort (row))
		{
			// Save selection and position
			SaveState ();
			dbg << "   sorting" << std::endl;
			Sort ();
			RepopulateView ();
			// Restore selection and position
			RestoreState ();
			dbg << "<-- TableBrowser::Update -- resorted - " << Table::GetName (_tableId) << std::endl;;
			return;
		}

		int item = RowToViewIndex (row);
		Assert (item != InvalidValue);
		dbg << "Redrawing with delay" << std::endl;
		_tableView.DelayRedrawItem (item);
		repaintRequired = true;
	}
	if (repaintRequired)
		_tableView.Repaint ();
	dbg << "   notifying tree" << std::endl;
	Notify ("tree");
	dbg << "<-- TableBrowser::Update - " << Table::GetName (_tableId) << std::endl;;
}

void TableBrowser::UpdateIfNecessary (char const *)
{
	dbg << "--> TableBrowser::UpdateIfNecessary - " << Table::GetName (_tableId) << std::endl;;
	// External notification from folder watcher received
	if (!_isVisible)
	{
		// Browser window is not visible -- just invalidate the record set
		dbg << "    Not visible" << std::endl;
		if (_recordSet.get () != 0)
		{
			dbg << "    Invalidating record set" << std::endl;
			_recordSet->Invalidate ();
		}
	}
	else
	{
		dbg << "    Visible view gets external notification -- something is already displayed" << std::endl;
		Assert (_recordSet.get () != 0);
		// Non empty visible view gets external notification
		TableBrowser::ViewRefresher refresher (*this);
		if (RefreshRecordSetIfNecessary (true))	// Notification
			refresher.UpdateView ();
	}
	// Notify our observers
	Notify ();
	dbg << "<-- TableBrowser::UpdateIfNecessary - " << Table::GetName (_tableId) << std::endl;;
}

bool TableBrowser::NeedSort (unsigned row)
{
	dbg << "--> TableBrowser::NeedSort with row " << row << std::endl;
	unsigned int sortCol;
    SortType sortOrder = _restriction.GetSortCol (sortCol);
	unsigned int item = RowToViewIndex (row);
	int prevRow = InvalidValue;
	if (item != 0)
		prevRow = _sortVector [item - 1];
	int nextRow = InvalidValue;
	if (item != _recordSet->RowCount () - 1)
		nextRow = _sortVector [item + 1];
	bool needSort = false;

	// The CmpRows return value indicates the relation of rowX to rowY as follows.
	//
	// Return Value     Description
	//    < 0           rowX less than rowY
	//      0           rowX identical to rowY
	//    > 0           rowX greater than rowY

	if (prevRow != InvalidValue)
	{
		int cmpResult = _recordSet->CmpRows (prevRow, row, sortCol);
		if (cmpResult != 0)
		{
			if (sortOrder == sortAscend)
				needSort = cmpResult > 0;
			else
				needSort = cmpResult < 0;
		}
	}

	if (!needSort && nextRow != InvalidValue)
	{
		int cmpResult = _recordSet->CmpRows (row, nextRow, sortCol);
		if (cmpResult != 0)
		{
			if (sortOrder == sortAscend)
				needSort = cmpResult > 0;
			else
				needSort = cmpResult < 0;
		}
	}
	dbg << "<-- TableBrowser::NeedSort returning " << needSort << std::endl;
	return needSort;
}

bool RangeTableBrowser::Sort ()
{
	dbg << "--> RangeTableBrowser::Sort - " << Table::GetName (_tableId) << std::endl;;
	unsigned int primarySortCol;
	SortType primarySortOrder = _restriction.GetSortCol (primarySortCol);
	if (primarySortOrder != sortNone && _sortVector.size () > 1)
	{
		Assert (primarySortCol != Table::colInvalid);
		Assert (_sortVector.size () == _recordSet->RowCount ());
		Assert (primarySortOrder == sortAscend || primarySortOrder == sortDescend);
		if (primarySortOrder == sortDescend && _sortVector [0] == 0 ||
			primarySortOrder == sortAscend && _sortVector [0] == _sortVector.size () - 1)
		{
			// Already sorted as needed
			dbg << "<-- RangeTableBrowser::Sort -- already sorted - " << Table::GetName (_tableId) << std::endl;;
			return true;
		}
		std::reverse (_sortVector.begin (), _sortVector.end ());
		_tableView.ClearRows ();
		dbg << "<-- RangeTableBrowser::Sort -- sorted - " << Table::GetName (_tableId) << std::endl;;
		return true;
	}
	dbg << "<-- RangeTableBrowser::Sort -- not sorted - " << Table::GetName (_tableId) << std::endl;;
	return false;
}

bool RangeTableBrowser::IsUpsideDown () const
{
	unsigned int primarySortCol;
	SortType primarySortOrder = _restriction.GetSortCol (primarySortCol);
	return (primarySortOrder == sortAscend);
}

void RangeTableBrowser::OnFirstRefresh ()
{
	_memento.reset (new ViewMemento (CurrentVersion));
}

void RangeTableBrowser::GetImage (unsigned iItem, int & iImage, int & iOverlay) const
{
	TableBrowser::GetImage (iItem, iImage, iOverlay);
	if (iImage != 0 && iImage != imageLabel && IsUpsideDown ())
		iImage += VersionIconCount; // turns image upside down
}
