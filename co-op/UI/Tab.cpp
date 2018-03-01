//----------------------------------
// (c) Reliable Software 2002 - 2006
//----------------------------------

#include "precompiled.h"
#include "Tab.h"
#include "resource.h"
#include <Graph/Icon.h>

PageTabs::PageTabs (Win::Dow::Handle hwndParent, int id)
	: EnumeratedTabs<ViewPage> (hwndParent, id), _images (5, 16, 2)
{
	Icon::SharedMaker icon (5, 16);
	_images.AddIcon (icon.Load (GetInstance (), I_TAB_RED));
	_images.AddIcon (icon.Load (GetInstance (), I_TAB_YELLOW));
	SetImageList (_images);
    AddTab (0, "Files", FilesPage);
    AddTab (1, "Check-in Area", CheckInAreaPage);
    AddTab (2, "Inbox", MailBoxPage);
    AddTab (3, "History", HistoryPage);
    AddTab (4, "Projects", ProjectPage);
}

PageTabs::~PageTabs ()
{
	SetImageList ();
}

void PageTabs::SetPageImage (ViewPage page, int imageIdx)
{
	int tabIdx = PageToIndex (page);
	Assert (tabIdx != -1);
	SetImage (tabIdx, imageIdx);
}

void PageTabs::RemovePageImage (ViewPage page)
{
	int tabIdx = PageToIndex (page);
	Assert (tabIdx != -1);
	RemoveImage (tabIdx);
}
