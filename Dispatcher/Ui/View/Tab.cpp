// ----------------------------------
// (c) Reliable Software, 2006 - 2007
// ----------------------------------

#include "precompiled.h"
#include "Tab.h"
#include "ViewType.h"
#include "resource.h"

MainWinTabs::MainWinTabs (Win::Dow::Handle hwndParent, int id)
  : EnumeratedTabs<ViewType> (hwndParent, id), _images (5, 16, 2)
{
	Tab::Maker tabMaker (hwndParent, id);
	tabMaker.Style () << Win::Style::Ex::ClientEdge;
	Reset (tabMaker.Create ());

	Icon::SharedMaker icon (5, 16);
	_images.AddIcon (icon.Load (GetInstance (), ID_TAB_RED));
	_images.AddIcon (icon.Load (GetInstance (), ID_TAB_YELLOW));
	SetImageList (_images);

	AddTab (0, "Quarantine",   QuarantineView);
	AddTab (1, "Alert Log",    AlertLogView);
	AddTab (2, "Public Inbox", PublicInboxView);
	AddTab (3, "Members",      MemberView);
	ResString tabName (hwndParent, IDS_REMOTE_TAB_NAME);
	AddTab (4, tabName,  RemoteHubView);
	SetCurSelection (0);
}

MainWinTabs::~MainWinTabs ()
{
	SetImageList ();
}

void MainWinTabs::SetPageImage (ViewType page, int imageIdx)
{
	int tabIdx = PageToIndex (page);
	Assert (tabIdx != -1);
	SetImage (tabIdx, imageIdx);
}

void MainWinTabs::RemovePageImage (ViewType page)
{
	int tabIdx = PageToIndex (page);
	Assert (tabIdx != -1);
	RemoveImage (tabIdx);
}
