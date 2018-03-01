//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "TablePreferences.h"

#include <StringOp.h>

char const * TablePreferences::_sortOrderValueName = "Sort order";
char const * TablePreferences::_sortIndexValueName = "Sort column";
char const * TablePreferences::_secondarySortOrderValueName = "Secondary sort order";
char const * TablePreferences::_secondarySortIndexValueName = "Secondary sort column";
char const * TablePreferences::_extFilterValueName = "Extension filter";
char const * TablePreferences::_optionsValueName = "Options";

TablePreferences::TablePreferences (Preferences::Storage const & storage,
									std::string const & tableName,
									Column::Info const  * columnInfo)
	: Preferences::Storage (storage, tableName),
	  _columnInfo (columnInfo),
	  _columnCount (0),
	  _sortOrder (sortNone),
	  _secondarySortOrder (sortNone),
	  _sortColumnIdx (-1),
	  _secondarySortColumnIdx (0)
{
	try
	{
		if (PathExists ())
		{
			for (Preferences::Storage::Sequencer seq (*this); !seq.AtEnd (); seq.Advance ())
			{
				std::string valueName = seq.GetValueName ();
				if (valueName == _sortOrderValueName && seq.IsValueLong ())
				{
					_sortOrder = static_cast<SortType>(seq.GetValueLong ());
				}
				else if (valueName == _sortIndexValueName && seq.IsValueLong ())
				{
					_sortColumnIdx = seq.GetValueLong ();
				}
				else if (valueName == _secondarySortOrderValueName && seq.IsValueLong ())
				{
					_secondarySortOrder = static_cast<SortType>(seq.GetValueLong ());
				}
				else if (valueName == _secondarySortIndexValueName && seq.IsValueLong ())
				{
					_secondarySortColumnIdx = seq.GetValueLong ();
				}
				else if (valueName == _extFilterValueName && seq.IsMultiValue ())
				{
					MultiString filterString;
					seq.GetMultiString (filterString);
					std::copy (filterString.begin (),
						filterString.end (),
						std::inserter (_filter, _filter.end ()));
				}
				else if (valueName == _optionsValueName && seq.IsMultiValue ())
				{
					seq.GetAsNamedBool (_options);
				}
				else if (seq.IsValueLong ())
				{
					// Column width
					unsigned long width = seq.GetValueLong ();
					_currentWidth [valueName] = width;
				}
			}
		}
		else
		{
			CreatePath ();
		}
	}
	catch ( ... )
	{
		// Ignore all exceptions
	}
	Verify ();
}

TablePreferences::~TablePreferences ()
{
	try
	{
		for (WidthMap::const_iterator iter = _currentWidth.begin ();
			 iter != _currentWidth.end ();
			 ++iter)
		{
			Save (iter->first, iter->second);
		}
		Save (_sortOrderValueName, _sortOrder);
		Save (_sortIndexValueName, _sortColumnIdx);
		Save (_secondarySortOrderValueName, _secondarySortOrder);
		Save (_secondarySortIndexValueName, _secondarySortColumnIdx);
		MultiString extensions;
		std::copy (_filter.begin (), _filter.end (), std::back_inserter (extensions));
		Save (_extFilterValueName, extensions);
		Save (_optionsValueName, _options);
	}
	catch ( ... )
	{
	}
}

void TablePreferences::Verify ()
{
	// Verify column widths
	for (_columnCount = 0; strcmp(_columnInfo [_columnCount].heading, "") != 0; ++_columnCount)
	{
		unsigned int columnDefaultPixelWidth = _columnInfo [_columnCount].defaultWidth * _cxChar;
		TablePreferences::WidthMap::iterator columnWidth =
			_currentWidth.find (_columnInfo [_columnCount].heading);
		if (columnWidth != _currentWidth.end ())
		{
			// Found column in the current width map
			if (columnWidth->second < columnDefaultPixelWidth / 4)
				columnWidth->second = columnDefaultPixelWidth;	// Column too narrow - reset default pixel width
		}
		else
		{
			_currentWidth.insert (std::make_pair (_columnInfo [_columnCount].heading,
												  columnDefaultPixelWidth));
		}
	}
	Assert (_columnCount != 0);
	// Empty table gets one column for at least 160 chars
	_emptyTableColumnWidth = 160 * _cxChar;
	// Verify sort column
	if (_sortColumnIdx == -1 || _sortColumnIdx >= _columnCount)
	{
		// Find first column with defined sort order
		for (unsigned i = 0; strcmp(_columnInfo [i].heading, "") != 0; ++i)
		{
			if (_columnInfo [i].sortOrder != sortNone)
			{
				_sortColumnIdx = i;
				break;
			}
		}
	}

	Assert (0 <= _sortColumnIdx && _sortColumnIdx < _columnCount);
	if (_sortOrder == sortNone)
	{
		// Primary sort order not defined
		if (_columnInfo [_sortColumnIdx].sortOrder != sortNone)
		{
			// Use default sort order defined for that column
			_sortOrder = _columnInfo [_sortColumnIdx].sortOrder;
		}
	}
	else if (_sortOrder != sortAscend && _sortOrder != sortDescend)
	{
		// Illegal sort order - use default sort order for that column
		_sortOrder = _columnInfo [_sortColumnIdx].sortOrder;
	}

	if (_secondarySortColumnIdx >= _columnCount)
		_secondarySortColumnIdx = 0;
	Assert (0 <= _secondarySortColumnIdx && _secondarySortColumnIdx < _columnCount);
	if (_secondarySortOrder != sortNone && _secondarySortOrder != sortAscend && _secondarySortOrder != sortDescend)
		_secondarySortOrder = sortNone;
	if (_secondarySortColumnIdx == _sortColumnIdx)
		_secondarySortOrder = sortNone;
}

void TablePreferences::SetSortColumn (unsigned int colIdx)
{
	Assert (colIdx < _columnCount);
	// Current sort order becomes secondary sort order
	_secondarySortOrder = _sortOrder;
	_secondarySortColumnIdx = _sortColumnIdx;
	// Switch to the new sort column
	_sortColumnIdx = colIdx;
	// Start with the default sort order for the new sort column
	_sortOrder = _columnInfo [_sortColumnIdx].sortOrder;
}

void TablePreferences::SetColumnWidth (unsigned int colIdx, unsigned int width)
{
	Assert (colIdx < _columnCount);
	_currentWidth [GetColumnHeading (colIdx)] = width;
}

void TablePreferences::ChangeCurrentSortOrder ()
{
	if (_sortOrder != sortNone)
	{
		_sortOrder = (_sortOrder == sortAscend ? sortDescend : sortAscend);
	}
}

void TablePreferences::GetSortInfo (unsigned int & colIdx, SortType & sortOrder) const
{
	colIdx = _sortColumnIdx;
	sortOrder = _sortOrder;
}

std::string const TablePreferences::GetColumnHeading (unsigned int colIdx) const
{
	Assert (colIdx < _columnCount);
	return _columnInfo [colIdx].heading;
}

Win::Report::ColAlignment TablePreferences::GetColumnAlignment (unsigned int colIdx) const
{
	Assert (colIdx < _columnCount);
	return _columnInfo [colIdx].alignment;
}

unsigned int TablePreferences::GetColumnWidth (std::string const & columnName) const
{
	WidthMap::const_iterator iter = _currentWidth.find (columnName);
	Assert (iter != _currentWidth.end ());
	return iter->second;
}

class IsEqualHeading : public std::unary_function<Column::Info const &, bool>
{
public:
	IsEqualHeading (std::string const & heading)
		: _heading (heading)
	{}

	bool operator () (Column::Info const & info) const
	{
		return info.heading == _heading;
	}

private:
	std::string const &	_heading;
};

Column::Info const * TablePreferences::FindDefaultInfo (std::string const & columnHeading) const
{
	Column::Info const * iter = 
		std::find_if (&_columnInfo [0], &_columnInfo [_columnCount], IsEqualHeading (columnHeading));
	if (iter ==  &_columnInfo [_columnCount])
		return 0;

	return iter;
}