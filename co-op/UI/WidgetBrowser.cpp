//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "WidgetBrowser.h"

void WidgetBrowser::SetFileFilter (std::unique_ptr<FileFilter> newFilter)
{
	FileFilter const * curFilter = _restriction.GetFileFilter ();
	bool changeFilter = (newFilter.get () == 0);	// Clear filtering
	if (!changeFilter)
	{
		Assert (newFilter.get () != 0);
		if (curFilter != 0)
			changeFilter = !curFilter->IsEqual (*newFilter);
		else
			changeFilter = newFilter->IsFilteringOn ();
	}

	if (changeFilter)
	{
		_restriction.SetFileFilter (std::move(newFilter));
		Invalidate ();
	}
}

bool WidgetBrowser::IsEmpty () const
{
	if (_recordSet.get () == 0 || !_recordSet->IsValid ())
	{
		return _tableProv.IsEmpty (_tableId);
	}
	else
	{
		Assert (_recordSet.get () != 0);
		return _recordSet->IsEmpty ();
	}
}

DegreeOfInterest WidgetBrowser::HowInteresting () const
{
	if (_recordSet.get () == 0 || !_recordSet->IsValid ())
	{
		return _tableProv.HowInteresting (_tableId);
	}
	else
	{
		Assert (_recordSet.get () != 0);
		return _recordSet->HowInteresting ();
	}
}

bool WidgetBrowser::IsDefaultSelection () const
{
	if (_recordSet.get () == 0 || !_recordSet->IsValid ())
		return false;

	return _recordSet->IsDefaultSelection ();
}

bool WidgetBrowser::SupportsDetailsView () const
{
	return _tableId == Table::projectTableId ||
		   _tableId == Table::folderTableId;
}