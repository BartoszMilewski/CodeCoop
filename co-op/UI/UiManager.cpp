//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "UiManager.h"
#include "UiManagerIds.h"
#include "CoopMsg.h"
#include "MenuTable.h"
#include "Global.h"
#include "resource.h"
#include "Predicate.h"
#include "MultiLine.h"
#include "FormattedText.h"
#include "OutputSink.h"
#include "Registry.h"

#include <Ctrl/Menu.h>
#include <Win/Metrics.h>
#include <Win/WinClass.h>

//----------
// UiManager
//----------

UiManager::UiManager (Win::Dow::Handle win, 
					  TableProvider & tableProvider, 
					  Cmd::Vector & cmdVector,
					  Cmd::Executor & executor)
	: _tableProvider (tableProvider),
	  _executor (executor),
	  _topWin (win),
	  _menu (Menu::barItems, cmdVector),
	  _instrumentBar (_topWin, cmdVector, executor),
	  _scriptBar (_topWin, cmdVector, executor),
	  _mergeBar (_topWin, cmdVector, executor),
	  _feedbackBar (_topWin),
	  _focusBarUser (_topWin.GetInstance ()),
	  _placement (_topWin, "Software\\Reliable Software\\Code Co-op\\Placement"),
#pragma warning (disable:4355)
	  _tabCtrl (win, this),
#pragma warning (default:4355)
	  _toolTipHandler (ID_TOOLBAR, _topWin),
	  _curPage (ProjectPage),
	  _cx (0),
	  _cy (0)
{
	Win::Class::Maker classMaker ("SimpleWinClass", win.GetInstance ());
    classMaker.SetBgBrush(Brush::Handle());
	classMaker.Register ();

	BuildBrowsingPageLayout ();
	BuildCheckInPageLayout ();
	BuildMailboxPageLayout ();
	BuildHistoryPageLayout ();
	BuildProjectPageLayout ();

	UseSystemSettings ();

	_curLayout = _layoutMap [_curPage];
	// View manager observes its Browsers, so it can
	// synchronize other GUI elements with changes in Browsers
	_layoutMap [MailBoxPage]->AttachObserver (this);
	_layoutMap [CheckInAreaPage]->AttachObserver (this);

	// Add button boxes to the tool tip controller
	_toolTipHandler.AddRebar (&_instrumentBar);
	_toolTipHandler.AddRebar (&_scriptBar);
	_toolTipHandler.AddRebar (&_mergeBar);
}

UiManager::~UiManager ()
{}

Notify::Handler * UiManager::GetNotifyHandler (Win::Dow::Handle winFrom, 
											   unsigned idFrom) throw ()
{
	Notify::Handler * handler = 0;
	if (_curLayout != 0)
		handler = _curLayout->GetNotifyHandler (winFrom, idFrom);
	if (handler != 0)
		return handler;
	else if (_tabCtrl.IsHandlerFor (idFrom))
		return &_tabCtrl;
	else if (_instrumentBar.IsHandlerFor (idFrom))
		return &_instrumentBar;
	else if (_scriptBar.IsHandlerFor (idFrom))
		return &_scriptBar;
	else if (_mergeBar.IsHandlerFor (idFrom))
		return &_mergeBar;
	else
		return &_toolTipHandler;
}

Control::Handler * UiManager::GetControlHandler (Win::Dow::Handle winFrom, 
												unsigned idFrom) throw ()
{
	Control::Handler * handler = _instrumentBar.GetControlHandler (winFrom, idFrom);
	if (handler != 0)
		return handler;

	handler = _scriptBar.GetControlHandler (winFrom, idFrom);
	if (handler != 0)
		return handler;

	return _mergeBar.GetControlHandler (winFrom, idFrom);
}

FeedbackManager * UiManager::GetFeedback ()
{
	FeedbackManager * pMan = &_feedbackBar;
	return pMan;
}

Table::Id UiManager::GetCurrentTableId () const
{
	return _curLayout->GetCurBrowser ()->GetTableId ();
}

Table::Id UiManager::GetMainTableId () const
{
	return _curLayout->GetMainTableId ();
}

Restriction const & UiManager::GetPresentationRestriction () const
{
	return _curLayout->GetMainTablePresentationRestriction ();
}

void UiManager::GetScrollBookmarks (std::vector<Bookmark> & bookmarks) const
{
	_curLayout->GetMainTableScrollBookmarks (bookmarks);
}

void UiManager::SetRestrictionFlag (std::string const & flagName, bool flagValue)
{
	_curLayout->SetMainTableRestrictionFlag (flagName, flagValue);
}

void UiManager::SetExtensionFilter (NocaseSet const & newFilter)
{
	_curLayout->SetMainTableExtensionFilter (newFilter);
}

void UiManager::SetScrollBookmarks (std::vector<Bookmark> const & bookmarks)
{
	_curLayout->SetMainTableScrollBookmarks (bookmarks);
}

void UiManager::SetInterestingItems (GidSet const & itemIds)
{
	_curLayout->SetMainTableInterestingItems (itemIds);
}

bool UiManager::CanShowHierarchy (bool & isVisible) const
{
	return _curLayout->CanShowHierarchy (isVisible);
}

void UiManager::ToggleHierarchyView ()
{
	_curLayout->ToggleHierarchyView ();
	_curLayout->Arrange ();
}

bool UiManager::CanShowDetails (bool & isVisible) const
{
	return _curLayout->CanShowDetails (isVisible);
}

void UiManager::ToggleDetailsView ()
{
	_curLayout->ToggleDetailsView (_feedbackBar);
	_curLayout->Arrange ();
}

RecordSet const * UiManager::GetRecordSet (Table::Id tableId) const
{
	return _curLayout->GetRecordSet (tableId);	// Find display pane showing specified table
}

RecordSet const * UiManager::GetMainRecordSet () const
{
	return _curLayout->GetMainRecordSet ();
}

RecordSet const * UiManager::GetCurRecordSet () const
{
	return _curLayout->GetCurRecordSet();
}

std::string UiManager::GetInputText () const
{
	return _instrumentBar.GetEditText ();
}

void UiManager::DumpRecordSet (std::ostream & out, Table::Id tableId, bool allRows)
{
	std::unique_ptr<WindowSeq> seq;
	if (allRows)
	{
		seq.reset (new AllSeq (this, tableId));
	}
	else
	{
		seq.reset (new SelectionSeq (this, tableId));
		if (tableId == Table::historyTableId)
			seq->ExpandRange ();
	}
	RecordSet const * recordSet = _curLayout->GetCurRecordSet ();
	Assert (recordSet != 0);
	Assert (recordSet->GetTableId () == tableId);
	recordSet->DumpColHeaders (out);
	for (; !seq->AtEnd (); seq->Advance ())
	{
		recordSet->DumpRow (out, seq->GetRow ());
	}
}

void UiManager::GetRows (std::vector<unsigned> & rows, Table::Id tableId) const
{
	WidgetBrowser const * browser = _curLayout->GetBrowser (tableId);
	Assert (browser != 0);
	Assert (browser->GetTableId () == tableId);
	browser->GetAllRows (rows);
}

void UiManager::GetSelectedRows (std::vector<unsigned> & rows) const
{
	WidgetBrowser const * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	browser->GetSelectedRows (rows);
}

void UiManager::GetSelectedRows (std::vector<unsigned> & rows, Table::Id tableId) const
{
	WidgetBrowser const * browser = _curLayout->GetBrowser (tableId);
	Assert (browser != 0);
	Assert (browser->GetTableId () == tableId);
	browser->GetSelectedRows (rows);
}

void UiManager::SelectAll ()
{
	WidgetBrowser * browse = _curLayout->GetCurBrowser ();
	if (browse != 0)
		browse->SelectAll ();
}

void UiManager::SelectIds (GidList const & ids, Table::Id tableId)
{
	WidgetBrowser * browse = _curLayout->GetBrowser (tableId);
	if (browse != 0)
		browse->SelectIds (ids);
}

void UiManager::SelectItems (std::vector<unsigned> const & items, Table::Id tableId)
{
	WidgetBrowser * browse = _curLayout->GetBrowser (tableId);
	if (browse != 0)
		browse->SelectItems (items);
}
void UiManager::SetRange (History::Range const & range)
{
	Assert (_curLayout != 0);
	_curLayout->SetMainTableRange (range);
}

void UiManager::VerifyRecordSet () const
{
	_curLayout->VerifyMainTableRecordSet ();
}

class InvalidateLayout
{
public:
	void operator() (Layout * layout)
	{
		layout->Invalidate ();
	}
};

// Fresh project
void UiManager::RefreshAll ()
{
	std::for_each (_layouts.begin (), _layouts.end (), InvalidateLayout ());
	// not the project layout
	RefreshToolBars ();
	RefreshPageTabs ();
}

void UiManager::Refresh (ViewPage page)
{
	RebuildIfNecessary (page);
	GetLayout (page).Invalidate ();
	if (_curPage == page)
	{
		RefreshToolBars ();
		RefreshPageTabs ();
	}
}

void UiManager::RefreshPane (ViewPage page, Table::Id id)
{
	GetLayout (page).Invalidate (id);
	if (_curPage == page)
	{
		RefreshToolBars ();
		RefreshPageTabs ();
	}
}

bool UiManager::IsEmptyPage (ViewPage page) const
{
	Layout const & layout = GetLayout (page);
	return layout.IsMainTableEmpty ();
}

// Hierarchy operations

void UiManager::GoUp ()
{
	_curLayout->GoUp ();
	Refresh (_curPage);
}

void UiManager::GoDown (std::string const & name)
{
	_curLayout->GoDown (name);
	Refresh (_curPage);
}

void UiManager::GoTo(Vpath const & vpath)
{
	_curLayout->GoTo(vpath);
	Refresh (_curPage);
}

void UiManager::GotoRoot ()
{
	_curLayout->GoToRoot ();
	Refresh (_curPage);
}

void UiManager::Navigate (std::string const & target, int scrollPos)
{
	_curLayout->Navigate (target, scrollPos);
}

void UiManager::SetFileFilter (ViewPage page, std::unique_ptr<FileFilter> newFilter)
{
	Layout & layout = GetLayout (page);
	layout.SetMainTableFileFilter (std::move(newFilter));
	if (_curPage == page)
		RefreshToolBars ();
}

class ClearLayout
{
public:
	ClearLayout (bool forGood)
		: _forGood (forGood)
	{}

	void operator() (Layout * layout)
	{
		layout->Clear (_forGood);
	}

private:
	bool	_forGood;
};

void UiManager::Rebuild (bool isWiki)
{
	std::map<ViewPage, Layout *>::const_iterator it = _layoutMap.find (BrowserPage);
	if (isWiki)
	{
		// Wiki folder detected.
		if (it == _layoutMap.end ())
			BuildBrowserPageLayout ();
		else
			it->second->Invalidate ();
	}
	else
	{
		// Check if Wiki Browser page layout exists
		// and close it.
		if (it != _layoutMap.end ())
			DestroyPageLayout (BrowserPage);
	}
}

void UiManager::RebuildIfNecessary (ViewPage newPage)
{
	std::map<ViewPage, Layout *>::const_iterator it = _layoutMap.find (newPage);
	if (it == _layoutMap.end ())
	{
		// Create layout for the new page
		if (newPage == BrowserPage)
		{
			BuildBrowserPageLayout ();
		}
		else if (newPage == SynchAreaPage)
		{
			BuildSynchPageLayout ();
			// View manager observes its Browsers, so it can
			// synchronize other GUI elements with changes in Browsers
			_layoutMap [SynchAreaPage]->AttachObserver (this);
		}
		else
		{
			Assert (newPage == ProjectMergePage);
			BuildProjectMergePageLayout ();
		}
	}
}

void UiManager::ClearAll (bool forGood)
{
	std::for_each (_layouts.begin (), _layouts.end (), ClearLayout (forGood));
	if (forGood)
	{
		DestroyPageLayout (SynchAreaPage);
		DestroyPageLayout (BrowserPage);
		DestroyPageLayout (ProjectMergePage);
		_instrumentBar.LayoutChange (Tool::NotInProject);
	}
	// Invalidate current page - LastPage is never displayed
	_curPage = LastPage;
}

unsigned int UiManager::SelCount () const
{
	WidgetBrowser const * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	return browser->SelCount ();
}

bool UiManager::IsDefaultSelection () const
{
	Table::Id mainTableId = _curLayout->GetMainTableId ();
	WidgetBrowser const * browser = _curLayout->GetBrowser (mainTableId);
	Assert (browser != 0);
	return browser->IsDefaultSelection ();
}

bool UiManager::FindIfSome (std::function<bool(long, long)> predicate) const 
{
	WidgetBrowser const * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	return browser->FindIfSome (predicate);
}

bool UiManager::FindIfSelected (std::function<bool(long, long)> predicate) const 
{
	WidgetBrowser const * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	return browser->FindIfSelected (predicate);
}

bool UiManager::FindIfSelected (Table::Id tableId, std::function<bool(long, long)> predicate) const
{
	WidgetBrowser const * browser = _curLayout->GetBrowser (tableId);
	Assert (browser != 0);
	return browser->FindIfSelected (predicate);
}

bool UiManager::FindIfSelected (GidPredicate const & predicate) const 
{
	WidgetBrowser const * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	return browser->FindIfSelected (predicate);
}

void UiManager::OnStartup ()
{
	ShowCurrentPage ();
	_feedbackBar.Close ();
}

void UiManager::OnSize (int cx, int cy)
{
	_cx = cx;
	_cy = cy;
	
	// TABS
	// Get display area where the tool bar will be shown
	Win::Rect tabRect (0, 0, cx, cy); // full client area rectangle
	_tabCtrl.GetView ().GetDisplayArea (tabRect); // rectangle, which we can use
	int tabsHeight = cy - tabRect.Height (); 
	// Instrument bar
	int toolHeight = _instrumentBar.Height ();
	// Shrink tabs window to allow only tool bar in its display area
	_tabCtrl.GetView ().ReSize  (0, 0, cx, tabsHeight + toolHeight);
	// Now shift instrument bar down to tabs' display area
	Win::Rect toolRect (0, tabsHeight, 0 + cx, tabsHeight + toolHeight); // tool bar rectangle
	_instrumentBar.Size (toolRect);

	// ListView, Splitter, and TreeView in one panel
	// (no good way to aggregate them as single control)
	int y = tabsHeight + toolHeight + 2;
	int statusHeight = _feedbackBar.Height ();
	int paneHeight = cy - y - statusHeight - 2;
	if (paneHeight < 0)
		paneHeight = 0;

	_layoutRect = Win::Rect (0, y, 0 + cx, y + paneHeight); // constructor not in terms of width/height

	// Feedback bar
	y += paneHeight;
	Win::Rect feedbackRect (0, y, _cx, y + statusHeight);
	_feedbackBar.Size (feedbackRect);

	_curLayout->Arrange ();	// Uses _layoutRect
}

// The application receives focus
void UiManager::OnFocus ()
{
	_curLayout->OnFocusSwitch ();
}

void UiManager::OnDestroy ()
{
	_curLayout->DeActivate ();
	while (_layoutMap.size () != 0)
	{
		Layout * layout = _layoutMap.begin ()->second;
		if (layout)
			layout->DetachObserver (this);
		_layoutMap.erase (_layoutMap.begin ());
	}
}

//
// Notification
//

void UiManager::TabChanged ()
{
	SelectPage (_tabCtrl.GetView ().GetSelection ());
}

void UiManager::SelectPage (ViewPage page)
{
	if (page == _curPage)
		return;
	try
	{
		if (_curLayout != 0)
			_curLayout->DeActivate ();
		RebuildIfNecessary (page);
		_curPage = page;
		_curLayout = &GetLayout (_curPage);
		ShowCurrentPage ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("UiManager: Unknown Error", Out::Error);
	}
	_feedbackBar.Close ();
}

void UiManager::SwitchPage (bool isForward)
{
	ViewPage nextPage = static_cast<ViewPage>((_curPage + (isForward ? 1 : LastPage - 1)) % LastPage);
	std::map<ViewPage, Layout *>::const_iterator it = _layoutMap.find (nextPage);
	while (it == _layoutMap.end ())
	{
		nextPage = static_cast<ViewPage>((nextPage + (isForward ? 1 : LastPage - 1)) % LastPage);
		it = _layoutMap.find (nextPage);
	}
	SelectPage (nextPage);
}

void UiManager::ShowCurrentPage ()
{
	Assert (_curLayout != 0);
	Assert (0 <= _curPage && _curPage < LastPage);
	_tabCtrl.GetView ().SetSelection (_curPage);
	_curLayout->Activate (_feedbackBar);
	_curLayout->Arrange ();
	RefreshToolBars (true);	// Force refresh even when layout pane doesn't have focus
	_curLayout->OnFocusSwitch ();
}

void UiManager::ClosePage (ViewPage page)
{
	DestroyPageLayout (page);
}

Layout const & UiManager::GetLayout (ViewPage page) const
{
	std::map<ViewPage, Layout *>::const_iterator iter = _layoutMap.find (page);
	Assert (iter != _layoutMap.end ());
	return *iter->second;
}

Layout & UiManager::GetLayout (ViewPage page)
{
	UiManager const * self = this;
	return const_cast<Layout &> (self->GetLayout (page));
}

void UiManager::InPlaceEdit (int row)
{
	Assert (_curLayout != 0);
	_curLayout->InPlaceEdit (row);
}

void UiManager::BeginNewFolderEdit ()
{
	Assert (_curLayout != 0);
	_curLayout->NewItemCreation ();
}

void UiManager::AbortNewFolderEdit ()
{
	Assert (_curLayout != 0);
	_curLayout->AbortNewItemCreation ();
}

void UiManager::SelectItem (int firstChar)
{
    WidgetBrowser * browser = _curLayout->GetCurBrowser ();
	Assert (browser != 0);
	int selectedItem = browser->SelectIf (StartsWithChar (firstChar));
	if (selectedItem != -1)
		browser->ScrollToItem (selectedItem);
}

void UiManager::MoveVSplitter (int value, unsigned splitterId)
{
	_curLayout->MoveVSplitter (value);
	_curLayout->Arrange ();
}

void UiManager::MoveHSplitter (int value, unsigned splitterId)
{
	_curLayout->MoveHSplitter (value);
	_curLayout->Arrange ();
}

void UiManager::UpdateBrowseWindow (TableBrowser *browse)
{
	browse->Invalidate ();
}

void UiManager::UpdateAll ()
{
	//	Watch out for deadlock!  Don't do anything that might cause a
	//	message to be sent.
	Win::UserMessage um (UM_REFRESH_VIEWTABS);
	_topWin.PostMsg (um);
}

void UiManager::SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows)
{
	_placement.PlaceWindow (cmdShow, multipleWindows);
}

void UiManager::RefreshPopup (Menu::Handle menu, int barItemNo)
{
	bool hideDisabled = false;
	if (!_menu.IsSubMenu (menu, barItemNo))
	{
		// Right mouse button clicked on the current item
		// Display popup context menu
		if (SelCount () != 0)
			barItemNo = _menu.GetBarItem ("Selection");
		else
			barItemNo = _menu.GetBarItem ("Folder");
		hideDisabled = true;
	}
	_menu.RefreshPopup (menu, barItemNo, hideDisabled);
}

bool UiManager::DisplayContextMenu (Win::Dow::Handle hwndCmd, Win::Dow::Handle hwndClicked, int x, int y)
{
	if (_curLayout->HasWindow (hwndClicked))
	{
		Menu::Tracker tracker;
		tracker.TrackMenu (hwndCmd, x, y);
		return true;
	}
	return false;
}

void UiManager::NonClientMetricsChanged ()
{
	// The metrics associated with the non-client area of non-minimized windows have changed.
	// Refresh our GUI.
	UseSystemSettings ();
	Win::Rect topWndRect;
	_topWin.GetClientRect (topWndRect);
	OnSize (topWndRect.Width (), topWndRect.Height ());
}

void UiManager::RefreshToolBars (bool force)
{
	if (_curLayout != 0)
		_curLayout->RefreshToolBars (force);
}

void UiManager::LayoutChange ()
{
	OnSize (_cx, _cy);
}

void UiManager::RefreshPageTabs ()
{
	// Mark tabs with icons for interesting pages
	for (TabSequencer pageSeq (_tabCtrl.GetView ()); !pageSeq.AtEnd (); pageSeq.Advance ())
	{
		ViewPage page = pageSeq.GetPage ();
		Layout const & layout = GetLayout (page);
		DegreeOfInterest type = layout.HowInterestingIsMainTable ();
		if (type == NotInteresting)
			_tabCtrl.RemoveImage (page);
		else if (type == Interesting)
			_tabCtrl.SetImage (page, 1);
		else if (type == VeryInteresting)
			_tabCtrl.SetImage (page, 0);
	}
}

void UiManager::UseSystemSettings ()
{
	NonClientMetrics metrics;
	if (metrics.IsValid ())
	{
		_tabCtrl.GetView ().SetFont (metrics.GetMenuFont ());
		_feedbackBar.SetFont (metrics.GetStatusFont ());
		_instrumentBar.SetFont (metrics.GetStatusFont ());
		_scriptBar.SetFont (metrics.GetStatusFont ());
		_mergeBar.SetFont (metrics.GetStatusFont ());
	}
}

void UiManager::DisableUI ()
{
	_topWin.Disable ();
	_instrumentBar.Disable ();
	Menu::Handle menu (_topWin);
	menu.Disable ();
	Menu::Handle::Refresh (_topWin);
}

void UiManager::EnableUI ()
{
	Menu::Handle menu (_topWin);
	menu.Enable ();
	Menu::Handle::Refresh (_topWin);
	_instrumentBar.Enable ();
	// Executing external command may affect GUI
	RefreshToolBars ();
	RefreshPageTabs ();
	_topWin.Enable ();
}

std::unique_ptr<Layout> UiManager::CreateLayout (Tool::BarLayout instrumentBarLayout,
											   std::string const & prefsBranchName)
{
	std::unique_ptr<Layout> layout
		(new Layout (_topWin,
					 _instrumentBar,
					 instrumentBarLayout,
					 _layoutRect,
					 _placement,
					 prefsBranchName));
	return layout;
}

std::unique_ptr<Pane::Bar> UiManager::CreateFocusBar (unsigned barId,
													unsigned associatedId,
													std::string const & title,
													Focus::Ring & focusRing)
{
	FocusBar::Ctrl * ctrl = _focusBarUser.MakeBarWindow (_topWin,
														 barId,
														 associatedId,
														 focusRing);
	ctrl->SetTitle (title);

	std::unique_ptr<Pane::Bar> focusBar (new Pane::FocusBar (ctrl));
	return focusBar;
}

void UiManager::BuildBrowsingPageLayout ()
{
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseFiles, "Browsing Page");

	// Create main display pane - file table display
	Layout::DragDropSupport ddSupport;
	ddSupport.set(Layout::DragBit);
	ddSupport.set(Layout::DropBit);
	TablePane * filesPane = layout->CreateTablePane (FILE_TABLE_CTRL_ID,
													 "Files",
													 Table::folderTableId,
													 Column::BrowsingFiles,
													 _tableProvider,
													 _executor,
													 ddSupport);	// Enable drag&drop support

	// Create details display pane - project table display
	ddSupport.clear();
	TablePane * projectPane = layout->CreateTablePane (MINI_PROJECT_TABLE_CTRL_ID,
													   "Active Projects",
													   Table::projectTableId,
													   Column::MiniProject,
													   _tableProvider,
													   _executor,
													   ddSupport);	// Don't support drag&drop
	
	projectPane->SetRestrictionFlag ("InterestingOnly");

	std::unique_ptr<Pane::Bar> projectFocusBar = CreateFocusBar (PROJECT_FOCUS_BAR_ID,
															   MINI_PROJECT_TABLE_CTRL_ID,
															   "Active Projects",
															   layout->GetFocusRing ());
	projectPane->AddBar (std::move(projectFocusBar));

	// Create hierarchy display pane - folder table display
	TreePane * folderPane = layout->CreateHierarchyPane (FOLDER_TABLE_CTRL_ID,
														 Table::folderTableId,
														 _tableProvider,
														 _executor);

	filesPane->Attach (folderPane->GetObserver (), "tree");
	std::unique_ptr<Pane::Bar> folderFocusBar = CreateFocusBar (FOLDER_FOCUS_BAR_ID,
															  FOLDER_TABLE_CTRL_ID,
															  "Project Folders",
															  layout->GetFocusRing ());
	
	folderPane->AddBar (std::move(folderFocusBar));

	_layoutMap [FilesPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildCheckInPageLayout ()
{
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseCheckInArea, "Check-in Page");

	// Create main display pane - file table display
	Layout::DragDropSupport ddSupport;
	ddSupport.set(Layout::DragBit);
	layout->CreateTablePane (CHECKINAREA_TABLE_CTRL_ID,
							 "Check In",
							 Table::checkInTableId,
							 Column::Checkin,
							 _tableProvider,
							 _executor,
							 ddSupport);	// Drag only

	_layoutMap [CheckInAreaPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildMailboxPageLayout ()
{
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseMailbox, "Mailbox Page");

	// Create main display pane - file table display
	layout->CreateRangeTablePane (MAILBOX_TABLE_CTRL_ID,
								  "Inbox",
								  Table::mailboxTableId,
								  Column::Mailbox,
								  _tableProvider,
								  _executor,
								  false);	// Multiple selection

	// Create script details pane
	Layout::DragDropSupport ddSupport;
	TablePane * scriptPane = layout->CreateTablePane (MAILBOX_SCRIPT_DETAILS_CTRL_ID,
													  "Incoming Script Details",
													  Table::scriptDetailsTableId,
													  Column::ScriptDetails,
													  _tableProvider,
													  _executor,
													  ddSupport);	// Don't support drag&drop

	std::unique_ptr<Pane::Bar> scriptToolBar (new Pane::ToolBar (_scriptBar));
	scriptPane->AddBar (std::move(scriptToolBar));

	_layoutMap [MailBoxPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildSynchPageLayout ()
{
	_tabCtrl.GetView ().InsertPageTab (SynchAreaPage, "Sync Merge");
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseSyncArea, "Sync Page");

	// Create main display pane - file table display
	Layout::DragDropSupport ddSupport;
	layout->CreateTablePane (SYNCAREA_TABLE_CTRL_ID,
							 "Synch",
							 Table::synchTableId,
							 Column::Synch,
							 _tableProvider,
							 _executor,
							 ddSupport);	// Don't support drag&drop

	_layoutMap [SynchAreaPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildHistoryPageLayout ()
{
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseHistory, "History Page");

	// Create main display pane - script table display
	TablePane * historyPane = layout->CreateRangeTablePane (HISTORY_TABLE_CTRL_ID,
															"History",
															Table::historyTableId,
															Column::History,
															_tableProvider,
															_executor,
															false);	// Multiple selection

	// Create script details pane
	Layout::DragDropSupport ddSupport;
	TablePane * scriptPane = layout->CreateTablePane (HISTORY_SCRIPT_DETAILS_CTRL_ID,
													  "Executed Script Details",
													  Table::scriptDetailsTableId,
													  Column::ScriptDetails,
													  _tableProvider,
													  _executor,
													  ddSupport);	// Don't support drag&drop

	std::unique_ptr<Pane::Bar> scriptToolBar (new Pane::ToolBar (_scriptBar));
	scriptPane->AddBar (std::move(scriptToolBar));

	_layoutMap [HistoryPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildProjectPageLayout ()
{
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseProjects, "Project Page");

	// Create main display pane - file table display
	Layout::DragDropSupport ddSupport;
	layout->CreateTablePane (PROJECT_TABLE_CTRL_ID,
							 "Projects",
							 Table::projectTableId,
							 Column::Project,
							 _tableProvider,
							 _executor,
							 ddSupport);	// Don't support drag&drop

	// Create details display pane - acctive project table display
	ddSupport.clear();
	TablePane * projectPane = layout->CreateTablePane (RECENT_PROJECT_TABLE_CTRL_ID,
													   "Active Projects",
													   Table::projectTableId,
													   Column::MiniProject,
													   _tableProvider,
													   _executor,
													   ddSupport);	// Don't support drag&drop
	
	projectPane->SetRestrictionFlag ("InterestingOnly");

	std::unique_ptr<Pane::Bar> projectFocusBar = CreateFocusBar (ACTIVE_PROJECT_FOCUS_BAR_ID,
															   RECENT_PROJECT_TABLE_CTRL_ID,
															   "Active Projects",
															   layout->GetFocusRing ());
	projectPane->AddBar (std::move(projectFocusBar));

	_layoutMap [ProjectPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildProjectMergePageLayout ()
{
	_tabCtrl.GetView ().InsertPageTab (ProjectMergePage, "Branch Merge");
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseProjectMerge, "Project Merge Page");

	// Create main display pane - script table display
	// Use multiple selection in merge view to allow multi-selective merges
	TablePane * historyPane = layout->CreateRangeTablePane (PROJECT_MERGE_HISTORY_CTRL_ID,
															"Project Merge",
															Table::historyTableId,
															Column::History,
															_tableProvider,
															_executor,
															false);	// Multiple selection

	// Create merge details pane
	Layout::DragDropSupport ddSupport;
	TablePane * scriptPane = layout->CreateTablePane (PROJECT_MERGE_SCRIPT_DETAILS_CTRL_ID,
													  "Merge Details",
													  Table::mergeDetailsTableId,
													  Column::MergeDetails,
													  _tableProvider,
													  _executor,
													  ddSupport);	// Don't support drag&drop

	std::unique_ptr<Pane::Bar> scriptToolBar (new Pane::ToolBar (_mergeBar));
	scriptPane->AddBar (std::move(scriptToolBar));

	_layoutMap [ProjectMergePage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::BuildBrowserPageLayout ()
{
	_tabCtrl.GetView ().InsertPageTab (BrowserPage, "Browser");
	std::unique_ptr<Layout> layout = CreateLayout (Tool::BrowseWiki, "Browser Page");

	// Create main display pane - file table display
	layout->CreateBrowserPane (BROWSER_CTRL_ID,
								Table::WikiDirectoryId,
								_tableProvider,
								_executor);

	_layoutMap [BrowserPage] = layout.get ();
	_layouts.push_back (std::move(layout));
}

void UiManager::DestroyPageLayout (ViewPage page)
{
	std::map<ViewPage, Layout *>::iterator iter = _layoutMap.find (page);
	if (iter == _layoutMap.end ())
		return;

	if (_curPage == page)
	{
		Assert (_curLayout != 0);
		_curLayout->DeActivate ();
		// Invalidate current page - LastPage is never displayed
		_curPage = LastPage;
		_curLayout = 0;
	}

	_tabCtrl.GetView ().RemovePageTab (page);
	for (unsigned i = 0; i < _layouts.size (); ++i)
	{
		if (_layouts [i] == iter->second)
		{
			_layouts.erase (i);
			break;
		}
	}
	_layoutMap.erase (page);
}
