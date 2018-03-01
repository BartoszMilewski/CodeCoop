// ----------------------------------
// (c) Reliable Software, 2000 - 2008
// ----------------------------------

#include "precompiled.h"
#include "ViewMan.h"
#include "MenuTable.h"
#include "AccelTable.h"
#include "ToolTable.h"
#include "Table.h"
#include "Globals.h"
#include "resource.h"
#include "DispatcherMsg.h"
#include <Win/MsgLoop.h>
#include <Win/Metrics.h>
#include <Win/Region.h>


const unsigned int QuarantineColumns  [] = { 250, 250 };
const unsigned int AlertLogColumns    [] = { 150, 50, 300 };
const unsigned int PublicInboxColumns [] = { 300, 100, 100 };
const unsigned int MemberColumns      [] = { 300, 100 };
const unsigned int ProjectMembersColumns [] = { 60, 140, 140, 80, 80 };
const unsigned int RemoteHubColumns   [] = { 200, 200, 100 };

ViewMan::ViewMan (Win::Dow::Handle win, 
				  Cmd::Vector & cmdVector, 
				  TableProvider & tableProvider, 
				  Cmd::Executor & executor)
  : _appWin (win),
    _cmdVector (cmdVector),
	_menu (Menu::barItems, _cmdVector),
	_tabs (win, ID_TABS),
	_toolbar (win, ID_TOOLBAR, ID_BUTTONS, Tool::DefaultButtonWidth, _cmdVector, Tool::Buttons),
	_buttonCtrl (ID_TOOLBAR, executor),
	_itemView (win, ID_MAINVIEW),
	_placement (_appWin, "Software\\Reliable Software\\Dispatcher\\Placement"),
	_quarantine (_itemView, tableProvider, QuarantineTableName, QuarantineColumns, 2),
	_alertLog (_itemView, tableProvider, AlertLogTableName, AlertLogColumns, 3),
	_publicInbox (_itemView, tableProvider, PublicInboxTableName, PublicInboxColumns, 3),
	_members (_itemView, tableProvider, MemberTableName, MemberColumns, 2),
	_projectMembers (_itemView, tableProvider, ProjectMemberTableName, ProjectMembersColumns, 5),
	_remoteHubs (_itemView, tableProvider, RemoteHubTableName, RemoteHubColumns, 3),
	_curView (0),
	_curTabViewStack (&_quarantineTabViewStack),
	_taskbarIconMenu (Menu::contextItems, _cmdVector)
{
	_toolbar.SetAllButtons ();
	_buttonCtrl.AddButtonIds (_toolbar.begin (), _toolbar.end ());
	Accel::Maker accelMaker (Accel::Keys, cmdVector);
	_kbdAccel.reset (new Accel::Handler (win, accelMaker.Create ()));
	_publicInboxTabViewStack.push (PublicInboxView);
	_quarantineTabViewStack.push (QuarantineView);
	_alertLogTabViewStack.push (AlertLogView);
	_memberTabViewStack.push (MemberView);
	_hubTabViewStack.push (RemoteHubView);

	_type2View [PublicInboxView] = &_publicInbox;
	_type2View [QuarantineView] = &_quarantine;
	_type2View [AlertLogView] = &_alertLog;
	_type2View [MemberView ] = &_members;
	_type2View [ProjectMemberView] = &_projectMembers;
	_type2View [RemoteHubView] = &_remoteHubs;

	_type2Stack [PublicInboxView] = &_publicInboxTabViewStack;
	_type2Stack [QuarantineView] = &_quarantineTabViewStack;
	_type2Stack [AlertLogView] = &_alertLogTabViewStack;
	_type2Stack [MemberView ] = &_memberTabViewStack;
	_type2Stack [RemoteHubView] = &_hubTabViewStack;

	Win::InfoDisplayMaker displayMaker (win, ID_TOOLBARTEXT);
	_caption.Reset (displayMaker.Create (_toolbar));
	_toolbar.AddWindow (_caption, win);

	Win::StatusBarMaker statusMaker (win, ID_STATUSBAR);
	statusMaker.Style () << Win::Style::Ex::ClientEdge;
	_statusBar.Reset (statusMaker.Create ());
	_statusBar.AddPart (50);
	ResetStatusText ();

	UseSystemSettings ();
}

void ViewMan::OnStartup ()
{
	_placement.PlaceWindow (Win::Hide);
}

void ViewMan::OnDestroy ()
{
	HideWindow ();
}

Control::Handler * ViewMan::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_buttonCtrl.IsCmdButton (idFrom))
		return &_buttonCtrl;
	return 0;
}

void ViewMan::AttachAccelerator (Win::Dow::Handle hwnd, Win::MessagePrepro * msgPrepro)
{
	msgPrepro->SetKbdAccelerator (_kbdAccel.get ());
}

void ViewMan::Size (int dx, int dy)
{
	// Tabs
	Win::Rect areaRect (0, 0, dx, dy);
	_tabs.GetDisplayArea (areaRect);
	int tabsHeight = dy - areaRect.Height ();
	// Toolbar
	_toolbar.AutoSize ();
	Win::ClientRect toolRect (_toolbar);
	int toolHeight = toolRect.Height ();
	_tabs.ReSize  (0, 0, dx, tabsHeight + toolHeight);
	int y = tabsHeight;
	toolRect.ShiftTo (0, y);
	_toolbar.Move (toolRect.left, toolRect.top, toolRect.Width (), toolRect.Height ());
	y += toolHeight;
	// Toolbar caption
	ResizeToolbarCaption ();
	// Main list view
    _itemView.ReSize (0, y, dx, dy - tabsHeight - toolHeight - _statusBar.Height ());
	// Status bar
    _statusBar.ReSize (0, dy - _statusBar.Height (), dx, _statusBar.Height ());
}

void ViewMan::RefreshAll ()
{
	_publicInbox.Invalidate ();
	_quarantine.Invalidate ();
	_alertLog.Invalidate ();
	_members.Invalidate ();
	_projectMembers.Invalidate ();
	_remoteHubs.Invalidate ();
	
	if (_curView != 0)
	{
		RefreshCurrentView ();
	}
}

void ViewMan::Refresh (ViewType viewType)
{
	Assert (viewType < ViewCount);
	TableView * view = _type2View [viewType];
	Assert (view != 0);
	view->Invalidate ();
	if (view == _curView)
	{
		RefreshCurrentView ();
	}
}

void ViewMan::Sort (unsigned int sortCol)
{
	Assert (_curView != 0);
	_curView->ReSort (sortCol);
}

void ViewMan::SetFocus ()
{
	_itemView.SetFocus ();
}

void ViewMan::ShowWindow ()
{
	if (_curView != 0)
		return;

	UpdateDisplay ();
}

void ViewMan::HideWindow ()
{
	if (_curView != 0)
	{
	    _curView->Hide ();
		_curView = 0;
	}
}

void ViewMan::SelectTab (ViewType newTab)
{
	Assert (_curView != 0);
	if (IsIn (newTab))
		return;

	SwitchTab (newTab);
	_tabs.SetCurSelection (newTab);
	UpdateDisplay ();
}

void ViewMan::TabChanged ()
{
	Assert (_curView != 0);
	SwitchTab (static_cast <ViewType> (_tabs.GetCurSelection ()));
	UpdateDisplay ();
}

void ViewMan::SwitchTab (ViewType newTab)
{
	Assert (newTab < TabCount);
	_curTabViewStack = _type2Stack [newTab];
}

void ViewMan::SwitchToNextTab (bool isForward)
{
	ViewType const currentView = GetCurViewType ();
	int const currentTab = _tabs.GetCurSelection ();
	ViewType const nextTab = static_cast<ViewType>((currentTab + (isForward ? 1 : TabCount - 1)) % TabCount);
	SelectTab (nextTab);
}

void ViewMan::GoUp ()
{
	Assert (_curView != 0);
	Assert (IsIn (ProjectMemberView));

	Assert (_curTabViewStack->size () > 1);
	_curTabViewStack->pop ();

	UpdateDisplay ();
}

void ViewMan::GoDown (Restriction const * restrict)
{
	Assert (_curView != 0);
	Assert (IsIn (MemberView));

	_curTabViewStack->push (ProjectMemberView);

	UpdateDisplay (restrict);
}

ViewType ViewMan::GetCurViewType () const 
{
	Assert (_curView != 0);
	return _curTabViewStack->top (); 
}

void ViewMan::UpdateDisplay (Restriction const * restrict)
{
	TableView * oldView = _curView;
	if (_curView != 0)
		_curView->Hide ();

	_curView = _type2View [_curTabViewStack->top ()];
	if (!_curView->Show (restrict))
	{
		_curView = oldView;
	}
	RefreshToolbar ();
	RefreshTabs ();
}

bool ViewMan::IsIn (ViewType view) const
{
	Assert (_curView != 0);
	return GetCurViewType () == view;
}

void ViewMan::UpdateToolBarState ()
{
	_toolbar.Refresh ();
}

void ViewMan::RefreshCurrentView ()
{
	Assert (_curView != 0);
	_curView->Refresh ();
	RefreshToolbar ();
	RefreshTabs ();
}

void ViewMan::RefreshToolbar ()
{
	Assert (_curView != 0);
	_caption.Hide ();
	_toolbar.SetLayout (Tool::LayoutTable [GetCurViewType ()]);
	UpdateToolBarState ();
	// the new button layout may contain different number of buttons
	// need to shift the _caption window
	ResizeToolbarCaption ();

	std::string captionTxt (_curView->GetCaption ());
	if (!captionTxt.empty ())
	{
		_caption.SetText (captionTxt.c_str ());
		_caption.Show ();
	}
}

void ViewMan::RefreshTabs ()
{
	int const tabCount = _tabs.GetCount ();
	for (int i = 0; i < tabCount; ++i)
	{
		ViewType tab = _tabs.GetParam<ViewType> (i);
		bool isInteresting = false;
		int image = 0;
		switch (tab)
		{
		case QuarantineView:
			isInteresting = _quarantine.IsInteresting ();
			image = 0;
			break;
		case AlertLogView:
			isInteresting = _alertLog.IsInteresting ();
			image = 0;
			break;
		case PublicInboxView:
			isInteresting = _publicInbox.IsInteresting ();
			image = 1;
			break;
		default:
			continue;
		};
		
		if (isInteresting)
			_tabs.SetPageImage (tab, image);
		else
			_tabs.RemovePageImage (tab);
	}
}

void ViewMan::ResizeToolbarCaption ()
{
	Win::ClientRect toolRect (_toolbar);
	int const toolHeight = toolRect.Height ();
	int const captionX = _toolbar.GetButtonsEndX () + 8;
	int const captionWidth = toolRect.Width () - captionX;
	int textLength, textHeight;
	_caption.GetTextSize ("Sample", textLength, textHeight);
	int const captionY = (toolHeight - textHeight) / 2;
	_caption.ReSize (captionX, captionY, captionWidth, textHeight);
}

void ViewMan::FillToolTip (Tool::TipForCtrl * tip) const
{
	_toolbar.FillToolTip (tip);
}

void ViewMan::FillToolTip (Tool::TipForWindow * tip) const
{
	Assert (_curView != 0);
	tip->SetText (_curView->GetCaption ().c_str ());
}

void ViewMan::RefreshPopup (Menu::Handle menu, int barItemNo)
{
	if (_taskbarIconMenu == menu)
	{
		_taskbarIconMenu.Refresh ();
	}
	else
	{
		bool hideDisabled = false;
		if (!_menu.IsSubMenu (menu, barItemNo))
		{
			// Right mouse button clicked on the current item
			// Display popup context menu -- always Selection menu.
			barItemNo = _menu.GetBarItem ("Selection");
			hideDisabled = true;
		}
		_menu.RefreshPopup (menu, barItemNo, hideDisabled);
	}
}

void ViewMan::DisplayTaskbarIconMenu ()
{
	Win::CursorPosPoint pt;
	// work around Windows bug: tracking notification icon context menu
	_appWin.SetForeground ();
	_taskbarIconMenu.TrackMenu (_appWin, pt.x, pt.y);
	_appWin.ForceTaskSwitch ();
}

bool ViewMan::DisplayContextMenu (Win::Dow::Handle hwndCmd, Win::Dow::Handle hwndClicked, int x, int y)
{
	if (hwndClicked == _curView->GetWindow ())
	{
		Menu::Tracker tracker;
		tracker.TrackMenu (hwndCmd, x, y);
		return true;
	}
	return false;
}

bool ViewMan::HasSelection () const
{
	return _curView == 0 ? false : _curView->HasSelection ();
}

void ViewMan::GetSelectedRows (std::vector<int> & rows) const
{
	Assert (_curView != 0);
    _curView->GetSelectedRows (rows);
}

RecordSet const * ViewMan::GetRecordSet (char const * tableName) const
{
	Assert (_curView != 0);
	return _curView->GetRecordSet ();
}

Restriction const & ViewMan::GetRestriction () const
{
	Assert (_curView != 0);
	return _curView->GetRestriction ();
}

void ViewMan::NonClientMetricsChanged ()
{
	// The metrics associated with the nonclient area of nonminimized windows have changed.
	// Refresh our GUI.
	UseSystemSettings ();
	Win::Rect topWinRect;
	_appWin.GetClientRect (topWinRect);
	Size (topWinRect.Width (), topWinRect.Height ());
}

void ViewMan::UseSystemSettings ()
{
	NonClientMetrics metrics;
	if (metrics.IsValid ())
	{
		_tabs.SetFont (metrics.GetMenuFont ());
		_caption.SetFont (metrics.GetStatusFont ());
		_statusBar.SetFont (metrics.GetStatusFont ());
	}
}
