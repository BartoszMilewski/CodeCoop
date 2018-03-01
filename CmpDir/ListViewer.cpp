//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "ListViewer.h"
#include "Image.h"

#include <Graph/Icon.h>

FileListView::FileListView (Win::Dow::Handle winParent, int id)
	: _colWidthPct (75),
	  _images (16, 16, imageLast)
{
	Win::ReportMaker reportMaker (winParent, id);
	reportMaker.Style () << Win::Style::Ex::ClientEdge;
	Init (reportMaker.Create ());
    for (int i = 0; i < imageLast; i++)
    {
		Icon::SharedMaker icon (16, 16);
		_images.AddIcon (icon.Load (GetInstance (), IconId [i]));
    }
    // Set overlay indices
    _images.SetOverlayImage (imageNew, overlayNew);
    _images.SetOverlayImage (imageDel, overlayDel);
    SetImageList (_images);
}

void FileListView::Initialize (int colWidthPct)
{
	_colWidthPct = colWidthPct;
}

void FileListView::Clear (bool isTwoCol)
{
	// remember col width
	CalcColWidthPct ();

	ClearAll ();
	_items.clear ();
	if (isTwoCol)
	{
		AddProportionalColumn (_colWidthPct, "Name");
		AddProportionalColumn (100 - _colWidthPct, "State");
	}
	else
		AddProportionalColumn (100, "Name");
}

void FileListView::Size (int x, int y, int width, int height)
{
	ReSize (x, y, width, height);
	// client area different size than width, hight
	Win::ClientRect rect (*this);
	width = rect.Width ();
	long colWidth = _colWidthPct * width;
	colWidth /= 100;
	SetColumnWidth (0, colWidth);
	SetColumnWidth (1, width - colWidth);
}

void FileListView::InitItem (ListView::Item & item, Data::Item const & data)
{
	DiffState state = data.GetDiffState ();

	if (data.IsFolder ())
		item.SetIcon (imageFolder);
	else if (data.IsDrive ())
		item.SetIcon (imageDrive);
	else if (state == stateDiff)
		item.SetIcon (imageDif);
	else
		item.SetIcon (imageFile);

	if (state == stateNew)
		item.SetOverlay (overlayNew);
	else if (state == stateDel)
		item.SetOverlay (overlayDel);
	else
		item.SetOverlay ();
}

void FileListView::UpdateStateCol (int pos, Data::Item const & data)
{
	ListView::Item item;
	item.SetSubItem (1); // state column
	DiffState state = data.GetDiffState ();

	if (state == stateNew)
		item.SetText ("Added");
	else if (state == stateDel)
		item.SetText ("Deleted");
	else if (state == stateDiff)
		item.SetText ("Changed");
	else // no change
	{
		if (data.IsFolder ())
			item.SetText ("In Both");
		else if (data.IsDrive ())
			item.SetText ("");
		else // identical file
			item.SetText ("Identical");
	}
	Win::ReportListing::UpdateItem (pos, item);
}

FileListView::Iterator FileListView::InsertItem (int pos, Iterator it, Data::Item const & data)
{
	ListView::Item item;
	std::string const & name = data.Name ();
	item.SetText (&name [0]);
	InitItem (item, data);
	Win::ReportListing::InsertItem (pos, item);

	UpdateStateCol (pos, data);

	return _items.insert (it, data);
}

void FileListView::UpdateItem (int pos, Iterator it, Data::Item const & data)
{
	ListView::Item item;
	std::string const & name = data.Name ();
	item.SetText (&name [0]);
	InitItem (item, data);
	Win::ReportListing::UpdateItem (pos, item);

	UpdateStateCol (pos, data);

	*it = data;
}

void FileListView::AppendItem (Data::Item const & data)
{
	ListView::Item item;
	std::string const & name = data.Name ();
	item.SetText (&name [0]);
	InitItem (item, data);
	int pos = Win::ReportListing::AppendItem (item);

	UpdateStateCol (pos, data);

	return _items.push_back (data);
}

long FileListView::CalcColWidthPct ()
{
	if (GetColCount () > 1)
	{
		Win::ClientRect rect (*this);
		int width = rect.Width ();
		int colWidth = GetColumnWidth (0);
		long widthPct = colWidth * 100;

		widthPct = widthPct / width;

		if (widthPct > 0 && widthPct < 100)
			_colWidthPct = widthPct;
	}
	return _colWidthPct;
}