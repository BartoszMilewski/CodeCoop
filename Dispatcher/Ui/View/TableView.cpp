//-----------------------------------------
// (c) Reliable Software 1998-2002
// ----------------------------------------
#include "precompiled.h"
#include "TableView.h"
#include "Table.h"
#include "resource.h"
#include "OutputSink.h"

#include <Win/Utility.h>

TableView::TableView (ItemView & itemView,
					  TableProvider & tableProv,
					  char const * tableName,
					  const unsigned int colWidths [],
					  const unsigned int colCount)
	: _tableProv (tableProv),
	  _tableName (tableName),
	  _needsRefreshing (true),
	  _itemView (itemView),
	  _viewSettings (tableName, colWidths, colCount),
	  _bottomItem (0),
	  _images (16, 16, 0)
{
}

bool TableView::Show (Restriction const * restrict)
{
	if (restrict != 0 && !_restriction.IsEqual (*restrict))
	{
		_restriction = *restrict;
		Invalidate ();
	}
	try
	{
		UpdateRecordSet ();
		RebuildView ();
		UpdateViewData ();
		RestoreState ();
	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException (e, _itemView);
		return false;
	}
	return true;
}

void TableView::Hide ()
{
	SavePreferences ();
	SaveState ();
}

void TableView::RebuildView ()
{
	_itemView.ClearAll ();
    for (unsigned int col = 0; col < _recordSet->GetColCount (); ++col)
    {
        _itemView.AddColumn (_viewSettings.GetColumnWidth (col), _recordSet->GetColumnHeading (col));
    }
	BindItemIcons ();
}

void TableView::Refresh ()
{
	SaveState ();
	UpdateRecordSet ();
	UpdateViewData ();
	RestoreState ();
}

void TableView::ReSort (unsigned int col)
{
	if (_viewSettings.GetSortCol () == col)
		return;

    _viewSettings.SetSortCol (col);
	SaveState ();
	_itemView.DeSelectAll ();
	if (Sort ())
	{
		UpdateViewData ();
		RestoreState ();
	}
}

// Returns true, if actually performed some sorting job 
bool TableView::Sort ()
{
	_sortVector.clear ();
	_sortVector.reserve (_recordSet->GetRowCount ());
	for (unsigned int i = 0; i < _recordSet->GetRowCount (); i++)
	{
		_sortVector.push_back (i);
	}
	if (_sortVector.size () > 1)
	{
		std::sort (_sortVector.begin (), 
				   _sortVector.end (), 
				   SortPredicate (_recordSet.get (), _viewSettings.GetSortCol ()));
		return true;
	}
    return false;
}

void TableView::UpdateRecordSet ()
{
	if (_needsRefreshing)
	{
		_recordSet = _tableProv.Query (_tableName, _restriction);
		_caption = _tableProv.QueryCaption (_tableName, _restriction);
		_needsRefreshing = false;
		Sort ();
	}
}

void TableView::UpdateViewData ()
{
    _itemView.ClearRows ();
    for (unsigned row = 0; row < _sortVector.size (); row++)
    {
        _itemView.AppendItem (_recordSet->GetFieldString (_sortVector [row], 0).c_str ());
        for (unsigned int col = 1; col < _recordSet->GetColCount (); col++)
        {
            _itemView.AddSubItem (_recordSet->GetFieldString (_sortVector [row], col).c_str (), 
							  row, col);
        }
    }
    for (unsigned row = 0; row < _sortVector.size (); row++)
        UpdateIcon (row);
}

void TableView::GetImage (unsigned int item, int & image, int & overlay) const
{
    _recordSet->GetImage (_sortVector [item], image, overlay);
}

void TableView::BindItemIcons ()
{
    std::vector<int> iconResId;
    _recordSet->GetItemIcons (iconResId);
	_images.SetCount (iconResId.size ());
    for (unsigned i = 0; i < iconResId.size (); i++)
    {
		Icon::SharedMaker icon (16, 16);
        _images.ReplaceIcon (i, icon.Load (_itemView.GetInstance (), iconResId [i]));
    }
    _itemView.SetImageList (_images);
}

void TableView::SavePreferences ()
{
    for (unsigned i = 0; i < _itemView.GetColCount (); i++)
    {
        _viewSettings.SetColumnWidth (i, _itemView.GetColumnWidth (i));
    }
    _viewSettings.SaveSettings ();    
}

void TableView::SaveState ()
{
	_bottomItem = _itemView.GetTopIndex () + _itemView.GetCountPerPage () - 1;
	if (_bottomItem < 0)
		_bottomItem = 0;
	else if (_bottomItem > _recordSet->GetRowCount ())
		_bottomItem = _recordSet->GetRowCount () - 1;

	std::vector<int> selection;
	GetSelectedRows (selection);
	if (selection.size () == 0)
	{
		_selectedRows.clear ();
	}
	else
	{
		_selectedRows.resize (selection.size ());
		for (unsigned int i = 0; i < selection.size (); ++i)
		{
			_recordSet->GetBookmark (selection [i], _selectedRows [i]);
		}
	}
}

void TableView::RestoreState ()
{
	if (_bottomItem < _recordSet->GetRowCount ())
		_itemView.EnsureVisible (_bottomItem);

	for (size_t i = 0; i < _selectedRows.size (); ++i)
	{
		int item = _recordSet->GetRow (_selectedRows [i]);
		if (item != -1)
		{
			_itemView.Select (RecordSetIdx2ViewRow (item));
		}
	}
}

void TableView::UpdateIcon (unsigned int itemIdx)
{
    int image, overlay;
    _recordSet->GetImage (_sortVector [itemIdx], image, overlay);

	Win::ListView::Item item;
	item.SetIcon (image);
	item.SetPos (itemIdx);
	item.SetOverlay (overlay);

	_itemView.SetItem (item);
}

bool TableView::HasSelection () const
{
    return _itemView.GetFirstSelected () != -1;
}

void TableView::GetSelectedRows (std::vector<int> & rows) const
{
	Assert (rows.empty ());
	rows.reserve (_itemView.GetSelectedCount ());
    int sel = _itemView.GetFirstSelected ();
    while (sel != -1)
    {
        rows.push_back (_sortVector [sel]);
        sel = _itemView.GetNextSelected (sel);
    }
}

bool TableView::IsInteresting ()
{
	UpdateRecordSet ();
	return _recordSet->GetRowCount () > 0;
}

unsigned int TableView::RecordSetIdx2ViewRow (unsigned int item) const
{
	Assert (item >= 0);
	Assert (item < _recordSet->GetRowCount ());
	for (unsigned int row = 0; row < _sortVector.size (); ++row)
	{
		if (_sortVector [row] == item)
			return row;
	}
	Assert ("!View item not found in sort vector");
	return -1;
}

bool TableView::SortPredicate::operator () (unsigned int row1, unsigned int row2) const
{
	Assert (row1 >= 0);
	Assert (row2 >= 0);
	Assert (row1 < _recordSet->GetRowCount ());
	Assert (row2 < _recordSet->GetRowCount ());
	
	return _recordSet->CmpRows (row1, row2, _col) < 0;
}

