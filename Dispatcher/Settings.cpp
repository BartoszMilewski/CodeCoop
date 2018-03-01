//---------------------------------------
//  (c) Reliable Software, 1998, 99, 2000
//---------------------------------------

#include "precompiled.h"
#include "Settings.h"
#include "Registry.h"

#include <LightString.h>
#include <Win/Win.h>
#include <StringOp.h>
#include <numeric>
//
// View Registry Settings
//

ViewSettings::ViewSettings (char const * viewName, 
							const unsigned int defaultColWidths [],
							const unsigned int colCount)
    : _viewName (viewName)
{
	Assert (defaultColWidths != 0);
	Assert (defaultColWidths [0] > 0);
	Assert (colCount > 0);
	_width.resize (colCount, 0);
	Registry::UserDispatcherPrefs userPrefs;
	userPrefs.GetColumnWidths (_viewName, _width);
	Assert (_width.size () == colCount);
	unsigned int const widthSum = std::accumulate (_width.begin (), _width.end (), 0);
	if (0 == widthSum)
	{
		// If all restored column widths are equal to zero
		// use default width for each column
		for (unsigned int i = 0; i < colCount; ++i)
			_width [i] = defaultColWidths [i];
	}
	else
	{
		// Zero is not allowed column width
		for (unsigned int i = 0; i < colCount; ++i)
		{
			if (_width [i] == 0)
				_width [i] = defaultColWidths [i];
		}
	}

	_sortCol = userPrefs.GetSortCol (_viewName);
}

void ViewSettings::SaveSettings ()
{
	Registry::UserDispatcherPrefs userPrefs;
	userPrefs.SetColumnWidths (_viewName, _width);
	userPrefs.SetSortCol (_viewName, _sortCol);
}

void ViewSettings::SetColumnWidth (unsigned int col, int width)
{
	if (col < _width.size ())
	{
		_width [col] = width;
	}
	else
	{
		Assert (col == _width.size ());
		_width.push_back (width);
	}
}

int ViewSettings::GetColumnWidth (unsigned int col) const
{
	Assert (col < _width.size ());
    return _width [col];
}
