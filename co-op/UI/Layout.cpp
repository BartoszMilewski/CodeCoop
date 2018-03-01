//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Layout.h"
#include "Controllers.h"
#include "TableBrowser.h"
#include "TreeBrowser.h"
#include "WikiBrowser.h"
#include "Predicate.h"
#include "FeedbackMan.h"
#include "InstrumentBar.h"
#include "Global.h"
#include "ColumnInfo.h"

#include <Ctrl/FocusBarWin.h>
#include <Win/Geom.h>
#include <Dbg/Out.h>

char const * Layout::Preferences::_hRatioValueName = "Horizontal ratio";
char const * Layout::Preferences::_vRatioValueName = "Vertical ratio";
char const * Layout::Preferences::_optionsValueName = "Options";

Layout::Preferences::Preferences (::Preferences::Storage const & root, std::string const & nodeName)
	: ::Preferences::Storage (root, nodeName),
	  _hSplitRatio (75),
	  _vSplitRatio (25)
{
	if (PathExists ())
	{
		Read (_hRatioValueName, _hSplitRatio);
		Read (_vRatioValueName, _vSplitRatio);
		Read (_optionsValueName, _options);
	}
	else
	{
		CreatePath ();
	}

	Verify ();
}

Layout::Preferences::~Preferences ()
{
	Save (_hRatioValueName, _hSplitRatio);
	Save (_vRatioValueName, _vSplitRatio);
	Save (_optionsValueName, _options);
}

void Layout::Preferences::Verify ()
{
	// Initial horizontal split ratio: 4/5 of window height
	if (_hSplitRatio == 0)
		_hSplitRatio = 80;
	else if (_hSplitRatio > 90)
		_hSplitRatio = 90;
	// Initial vertical split ratio: 1/20 of window width
	if (_vSplitRatio == 0)
		_vSplitRatio = 5;
	else if (_vSplitRatio > 75)
		_vSplitRatio = 75;
}

void Layout::Preferences::SetHRatio (unsigned ratio)
{
	_hSplitRatio = ratio;
}

void Layout::Preferences::SetVRatio (unsigned ratio)
{
	_vSplitRatio = ratio;
}

void Layout::Preferences::SetOptions (bool hiddenVSplitter, bool hiddenHSplitter)
{
	_options.Set ("HiddenVSplitter", hiddenVSplitter);
	_options.Set ("HiddenHSplitter", hiddenHSplitter);
}

// Layout functors

class ClearPane : public std::unary_function<Pane const *, void>
{
public:
	ClearPane (bool forGood)
		: _forGood (forGood)
	{}

	void operator() (Pane * pane)
	{
		pane->Clear (_forGood);
	}

private:
	bool	_forGood;
};

class IsEqualId : public std::unary_function<Pane const *, bool>
{
public:
	IsEqualId (unsigned id)
		: _id (id)
	{}

	bool operator () (Pane const * pane) const
	{
		return pane->GetId () == _id;
	}

private:
	unsigned	_id;
};

class IsEqualTableId : public std::unary_function<Pane const *, bool>
{
public:
	IsEqualTableId (Table::Id id)
		: _id (id)
	{}

	bool operator () (Pane const * pane) const
	{
		return pane->HasTable (_id);
	}

private:
	Table::Id	_id;
};

class HasWindowHandle : public std::unary_function<Pane const *, bool>
{
public:
	HasWindowHandle (Win::Dow::Handle h)
		: _h (h)
	{}

	bool operator () (Pane const * pane) const
	{
		return pane->HasWindow(_h);
	}

private:
	Win::Dow::Handle	_h;
};

class FocusSwitch : public std::unary_function<Pane const *, void>
{
public:
	FocusSwitch (unsigned focusId)
		: _focusId (focusId)
	{}

	void operator() (Pane * pane)
	{
		pane->OnFocus (_focusId);
	}

private:
	unsigned	_focusId;
};

Layout::Layout (Win::Dow::Handle parentWin,
				InstrumentBar & instrumentBar,
				Tool::BarLayout instrumentBarLayoutId,
				Win::Rect const & layoutRect,
				::Preferences::Storage const & root,
				std::string const & prefBranchName)
	: _parentWin (parentWin),
	  _preferences (root, prefBranchName),
	  _instrumentBar (instrumentBar),
	  _instrumentBarLayoutId (instrumentBarLayoutId),
	  _layoutRect (layoutRect),
	  _vSplitterUser (parentWin.GetInstance ()),
	  _hSplitterUser (parentWin.GetInstance ()),
	  _hiddenVSplitter (true),
	  _hiddenHSplitter (true),
	  _dragSourcePane(0)
{
	_focusRing.SetSink (this);
}

//
// Display pane construction
//

TablePane * Layout::CreateTablePane (unsigned paneId,
									 std::string const & paneName,
									 Table::Id tableId,
									 Column::Info const * columnInfo,
									 TableProvider & tableProvider,
									 Cmd::Executor & executor,
									 DragDropSupport isDragDrop)
{
	std::unique_ptr<TableController> ctrl;
	if (isDragDrop.test(DropBit))
	{
		ctrl.reset (new FileDropTableController (paneId,
												 _parentWin,
												 executor,
												 _focusRing,
												 isDragDrop.test(DragBit),
												 false));	// Multiple selection
	}
	else
	{
		ctrl.reset (new TableController (paneId,
									     _parentWin,
									     executor,
									     _focusRing,
										 isDragDrop.test(DragBit),
									     false));	// Multiple selection
	}

	std::unique_ptr<TableBrowser> browser
		(new TableBrowser (tableProvider,
						   tableId,
						   _preferences,
						   paneName,
						   columnInfo,
						   ctrl->GetView ()));

	ListObserver * listObserver = this;
	ctrl->Activate (browser.get (), listObserver);
	std::unique_ptr<TablePane> pane (new TablePane (std::move(ctrl), std::move(browser)));
	TablePane * tablePane = pane.get();
	AddPane (std::move(pane));
	return tablePane;
}

TablePane * Layout::CreateRangeTablePane (unsigned paneId,
										  std::string const & paneName,
										  Table::Id tableId,
										  Column::Info const * columnInfo,
										  TableProvider & tableProvider,
										  Cmd::Executor & executor,
										  bool isSingleSelection)
{
	std::unique_ptr<TableController> ctrl
		(new RangeTableController (paneId,
								   _parentWin,
								   executor,
								   _focusRing,
								   isSingleSelection));
	std::unique_ptr<TableBrowser> browser
		(new RangeTableBrowser (tableProvider,
								tableId,
								_preferences,
								paneName,
								columnInfo,
								ctrl->GetView ()));

	ListObserver * listObserver = this;
	ctrl->Activate (browser.get (), listObserver);
	std::unique_ptr<TablePane> pane (new TablePane (std::move(ctrl), std::move(browser)));
	TablePane * tablePane = pane.get();
	AddPane (std::move(pane));
	return tablePane;
}

TreePane * Layout::CreateHierarchyPane (unsigned paneId,
										Table::Id tableId,
										TableProvider & tableProvider,
										Cmd::Executor & executor)
{
	std::unique_ptr<HierarchyController> ctrl
		(new HierarchyController (paneId,
								  _parentWin,
								  executor,
								  _focusRing));
	std::unique_ptr<TreeBrowser> browser
		(new TreeBrowser (tableProvider,
						  tableId,
						  ctrl->GetView ()));

	ListObserver * listObserver = this;
	ctrl->Activate (browser.get (), listObserver);
	std::unique_ptr<TreePane> pane (new TreePane (std::move(ctrl), std::move(browser)));
	TreePane * treePane = pane.get ();
	AddPane (std::move(pane));
	return treePane;
}

BrowserPane * Layout::CreateBrowserPane (unsigned paneId,
										Table::Id tableId,
										TableProvider & tableProvider,
										Cmd::Executor & executor)
{
	std::unique_ptr<WikiBrowserController> ctrl
		(new WikiBrowserController (paneId,
							  _parentWin,
							  executor,
							  _focusRing));
	std::unique_ptr<WikiBrowser> browser (new WikiBrowser (tableProvider, tableId, ctrl->GetView ()));
	ctrl->Activate (browser.get ());
	std::unique_ptr<BrowserPane> pane (new BrowserPane (std::move(ctrl), std::move(browser)));
	BrowserPane * browserPane = pane.get ();
	AddPane (std::move(pane));
	return browserPane;
}

void Layout::AddPane (std::unique_ptr<Pane> pane)
{
	Assert (_panes.size () < 3);
	if (_panes.size () == 1)
	{
		// Adding second pane also adds horizontal splitter
		_hSplitter = _hSplitterUser.MakeSplitter (_parentWin, HSPLITTER_ID);
		_hiddenHSplitter = false;
	}
	else if (_panes.size () == 2)
	{
		// Adding third pane also adds vertical splitter
		_vSplitter = _vSplitterUser.MakeSplitter (_parentWin, VSPLITTER_ID);
		_hiddenVSplitter = false;
	}
	// First added pane is treated as the main pane
	_focusRing.AddId (pane->GetId ());
	_panes.push_back (std::move(pane));
}

void Layout::AttachObserver (Observer * observer)
{
	Assert (_panes.size () != 0);
	_panes [0]->Attach (observer);
}

void Layout::DetachObserver (Observer * observer)
{
	Assert (_panes.size () != 0);
	_panes [0]->Detach (observer);
}

void Layout::BeginDrag(Win::Dow::Handle win, unsigned id)
{
	auto_vector<Pane>::iterator it = std::find_if(_panes.begin(), _panes.end(), IsEqualId(id));
	Assume (it != _panes.end(), "BeginDrag called with unknown id");
	_dragSourcePane = *it;
}

void Layout::EndDrag() 
{
	_dragSourcePane = 0; 
}

//
// Direct item editing (label editing)
//

void Layout::InPlaceEdit (int row)
{
	Pane * pane = GetFocusPane ();
	pane->InPlaceEdit (row);
}

void Layout::NewItemCreation ()
{
	// New item is added to the main layout display pane
	Assert (_panes.size () != 0);
	_panes [0]->NewItemCreation ();
}

void Layout::AbortNewItemCreation ()
{
	Assert (_panes.size () != 0);
	_panes [0]->AbortNewItemCreation ();
}

//
// Main pane manipulation
//

Table::Id Layout::GetMainTableId () const
{
	Assert (_panes.size () != 0);
	WidgetBrowser const * browser = _panes [0]->GetBrowser ();
	return browser->GetTableId ();
}

unsigned Layout::GetMainPaneId () const
{
	Assert (_panes.size () != 0);
	return _panes [0]->GetId ();
}

Restriction const & Layout::GetMainTablePresentationRestriction () const
{
	Assert (_panes.size () != 0);
	WidgetBrowser const * browser = _panes [0]->GetBrowser ();
	return browser->GetPresentationRestriction ();
}

void Layout::GetMainTableScrollBookmarks (std::vector<Bookmark> & bookmarks) const
{
	Assert (_panes.size () != 0);
	WidgetBrowser const * browser = _panes [0]->GetBrowser ();
	browser->GetScrollBookmarks (bookmarks);
}

bool Layout::HasTable(Table::Id tableId) const
{
	for (auto_vector<Pane>::const_iterator iter = _panes.begin (); iter != _panes.end (); ++iter)
	{
		Pane * pane = *iter;
		if (pane->HasTable(tableId))
			return true;
	}
	return false;
}

void Layout::VerifyMainTableRecordSet () const
{
	Assert (_panes.size () != 0);
	Pane const * pane = _panes [0];
	pane->VerifyRecordSet();
#if 0
	RecordSet const * recordSet = pane->GetRecordSet ();
	// Revisit: Browser page doesn't have a record set yet
	if (recordSet != 0 && recordSet->IsValid ())
	{
#if !defined (NDEBUG)
		if (_panes.size () == 3)
		{
			Pane const * treeViewPane = _panes [2];
			WidgetBrowser const *  widgetBrowser = treeViewPane->GetBrowser ();
			TreeBrowser const * constTreeBrowser = dynamic_cast<TreeBrowser const *> (widgetBrowser);
			Assert (constTreeBrowser != 0);
			TreeBrowser * treeBrowser = const_cast<TreeBrowser *>(constTreeBrowser);
			Hierarchy & hierarchy = treeBrowser->GetHierarchy ();
			GlobalId selectedFolderId;
			std::string selectedFolderName;
			bool isTreeViewSelection = hierarchy.GetSelection (selectedFolderId,
															   selectedFolderName);
			Assert (isTreeViewSelection);
			if (recordSet->RowCount () != 0)
			{
				GlobalId parentId = recordSet->GetParentId (0);
				Assert (selectedFolderId == parentId ||
						selectedFolderId == gidInvalid ||
						parentId == gidInvalid);
			}
		}
#endif
		recordSet->Verify ();
	}
#endif
}

bool Layout::IsMainTableEmpty () const
{
	Assert (_panes.size () != 0);
	WidgetBrowser const * browser = _panes [0]->GetBrowser ();
	return browser->IsEmpty ();
}

DegreeOfInterest Layout::HowInterestingIsMainTable () const
{
	Assert (_panes.size () != 0);
	WidgetBrowser const * browser = _panes [0]->GetBrowser ();
	return browser->HowInteresting ();
}

void Layout::SetMainTableRestrictionFlag (std::string const & name, bool val)
{
	Assert (_panes.size () != 0);
	_panes [0]->SetRestrictionFlag (name, val);
}

void Layout::SetMainTableExtensionFilter (NocaseSet const & newFilter)
{
	Assert (_panes.size () != 0);
	WidgetBrowser * browser = _panes [0]->GetBrowser ();
	browser->SetExtensionFilter (newFilter);
}

void Layout::SetMainTableScrollBookmarks (std::vector<Bookmark> const & bookmarks)
{
	Assert (_panes.size () != 0);
	WidgetBrowser * browser = _panes [0]->GetBrowser ();
	browser->SetScrollBookmarks (bookmarks);
}

void Layout::SetMainTableInterestingItems (GidSet const & itemIds)
{
	Assert (_panes.size () != 0);
	WidgetBrowser * browser = _panes [0]->GetBrowser ();
	browser->SetInterestingItems (itemIds);
}

void Layout::SetMainTableFileFilter (std::unique_ptr<FileFilter> newFilter)
{
	Assert (_panes.size () != 0);
	WidgetBrowser * browser = _panes [0]->GetBrowser ();
	browser->SetFileFilter (std::move(newFilter));
}

void Layout::SetMainTableRange (History::Range const & range)
{
	Assert (_panes.size () != 0);
	WidgetBrowser * browser = _panes [0]->GetBrowser ();
	browser->SetRange (range);
	for (unsigned i = 1; i < _panes.size (); ++i)
	{
		_panes [i]->Invalidate ();
	}
}

void Layout::Activate (FeedbackManager & feedback)
{
	_instrumentBar.LayoutChange (_instrumentBarLayoutId);
	for (auto_vector<Pane>::iterator iter = _panes.begin (); iter != _panes.end (); ++iter)
	{
		Pane * pane = *iter;
		if (_focusRing.HasId (pane->GetId ()))
			pane->Activate (feedback);
	}
}

void Layout::DeActivate ()
{
	for (auto_vector<Pane>::iterator iter = _panes.begin (); iter != _panes.end (); ++iter)
	{
		Pane * pane = *iter;
		if (_focusRing.HasId (pane->GetId ()))
			pane->DeActivate ();
	}
	if (!_hSplitter.IsNull ())
		_hSplitter.Hide ();
	if (!_vSplitter.IsNull ())
		_vSplitter.Hide ();
}

static inline void InvalidatePane (Pane * pane)
{
	pane->Invalidate ();
}

void Layout::Invalidate ()
{
	std::for_each (_panes.begin (), _panes.end (), InvalidatePane);
}


void Layout::Invalidate (Table::Id tableId)
{
	auto_vector<Pane>::iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualTableId (tableId));
	while (iter != _panes.end ())
	{
		Pane * pane = *iter;
		pane->Invalidate ();
		++iter;
		iter = std::find_if (iter, _panes.end (), IsEqualTableId (tableId));
	}
}

void Layout::Clear (bool forGood)
{
	std::for_each (_panes.begin (), _panes.end (), ClearPane (forGood));
	if (forGood)
	{
		// Reset focus to the main table
		Assert (_panes.size () != 0);
		_focusRing.SwitchToThis (_panes [0]->GetId ());
	}
}

void Layout::Arrange ()
{
	if (_panes.size () == 1)
	{
		Pane * pane = _panes [0];
		pane->Move (_layoutRect);
	}
	else if (_panes.size () == 2)
	{
		// Horizontal split
		Win::Rect upperPane;
		Win::Rect lowerPane;
		PositionHorizontalSplitter (_layoutRect, upperPane, lowerPane);
		_panes [0]->Move (upperPane);
		_panes [1]->Move (lowerPane);
	}
	else
	{
		Assert (_panes.size () == 3);
		// Horizontal and vertical split
		Win::Rect leftPane;
		Win::Rect rightPane;
		PositionVerticalSplitter (_layoutRect, leftPane, rightPane);
		Win::Rect upperPane;
		Win::Rect lowerPane;
		PositionHorizontalSplitter (rightPane, upperPane, lowerPane);
		_panes [0]->Move (upperPane);
		_panes [1]->Move (lowerPane);
		_panes [2]->Move (leftPane);
	}
}

void Layout::RefreshToolBars (bool force)
{
	_instrumentBar.RefreshButtons ();
	Assert (_panes.size () != 0);
	WidgetBrowser const * mainBrowser = _panes [0]->GetBrowser ();
	if (mainBrowser != 0)
	{
		_instrumentBar.RefreshTextField (mainBrowser->GetCaption ());
		if (_panes.size () > 1)
			_panes [1]->RefreshBar (_focusRing.GetFocusId (), force);
	}
}

void Layout::GoUp ()
{
	if (_panes.size () == 3)
	{
		TreeBrowser * treeBrowser = reinterpret_cast<TreeBrowser *>(_panes [2]->GetBrowser ());
		Assert (treeBrowser != 0);
		treeBrowser->GetHierarchy ().GoUp ();
	}
}

void Layout::GoDown (std::string const & name)
{
	if (_panes.size () == 3)
	{
		TreeBrowser * treeBrowser = reinterpret_cast<TreeBrowser *>(_panes [2]->GetBrowser ());
		Assert (treeBrowser != 0);
		treeBrowser->GetHierarchy ().GoDown (name);
	}
}

void Layout::GoToRoot ()
{
	if (_panes.size () == 3)
	{
		TreeBrowser * treeBrowser = reinterpret_cast<TreeBrowser *>(_panes [2]->GetBrowser ());
		Assert (treeBrowser != 0);
		treeBrowser->GetHierarchy ().GoToRoot ();
	}
}

void Layout::GoTo(Vpath const & vpath)
{
	if (_panes.size () == 3)
	{
		TreeBrowser * treeBrowser = reinterpret_cast<TreeBrowser *>(_panes [2]->GetBrowser ());
		Assert (treeBrowser != 0);
		treeBrowser->GetHierarchy ().GoTo(vpath);
	}
}

bool Layout::CanShowHierarchy (bool & isVisible) const
{
	if (_panes.size () < 3)
	{
		isVisible = false;
		return false;
	}
	else
	{
		Assert (_panes.size () == 3);
		isVisible = !_hiddenVSplitter;
		// REVISIT: why we consult the pane browser if it supports hierarchy?
		// Under what condition the layout will contain left hierarchy pane, but
		// pane browser will not support hierarchy view?
		TreeBrowser const * treeBrowser = reinterpret_cast<TreeBrowser const *>(_panes [2]->GetBrowser ());
		Assert (treeBrowser != 0);
		return treeBrowser->IsActive ();
	}
}

void Layout::ToggleHierarchyView ()
{
	if (_panes.size () < 3)
		return;	// Nothing to toggle

	Assert (_panes.size () == 3);
	_hiddenVSplitter = !_hiddenVSplitter;
	Pane * hierarchyPane = _panes [2];
	if (_hiddenVSplitter)
		_focusRing.RemoveId (hierarchyPane->GetId ());
	else
		_focusRing.AddId (hierarchyPane->GetId ());
}

void Layout::Navigate (std::string const & name, int scrollPos)
{
	_panes [0]->Navigate (name, scrollPos);
}

bool Layout::CanShowDetails (bool & isVisible) const
{
	WidgetBrowser const * mainTableBrowser = GetBrowser (GetMainTableId ());
	Assert (mainTableBrowser != 0);
	if (mainTableBrowser->SupportsDetailsView ())
	{
		isVisible = !_hiddenHSplitter;
		return true;
	}
	isVisible = false;
	return false;
}

void Layout::ToggleDetailsView (FeedbackManager & feedback)
{
	Assert (_panes.size () != 0);
	if (_panes.size () == 1)
		return;	// Nothing to toggle

	Assert (_panes.size () <= 3);
	_hiddenHSplitter = !_hiddenHSplitter;
	Pane * detailsPane = _panes [1];
	if (_hiddenHSplitter)
	{
		_focusRing.RemoveId (detailsPane->GetId ());
		detailsPane->DeActivate ();
	}
	else
	{
		_focusRing.AddId (detailsPane->GetId ());
		detailsPane->Activate (feedback);
	}
}

void Layout::MoveVSplitter (int pixelPos)
{
	int ratio = pixelPos * 100 / _layoutRect.Width ();
	// prevent the split ratio from being too small or too big
	if (ratio < 3)
		ratio = 3;
	else if (ratio > 75)
		ratio = 75;

	_preferences.SetVRatio (ratio);
}

void Layout::MoveHSplitter (int pixelPos)
{
	int ratio = pixelPos * 100 / _layoutRect.Height ();
	// prevent the split ratio from being too small or too big
	if (ratio < 20)
		ratio = 20;
	else if (ratio > 95)
		ratio = 95;

	_preferences.SetHRatio (ratio);
}

Notify::Handler * Layout::GetNotifyHandler (Win::Dow::Handle winFrom, 
											unsigned idFrom) throw ()
{
	auto_vector<Pane>::iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualId (idFrom));
	if (iter != _panes.end ())
		return (*iter)->GetNotifyHandler ();

	return 0;
}

bool Layout::HasWindow (Win::Dow::Handle hwndClicked)
{
	auto_vector<Pane>::const_iterator iter =
		std::find_if (_panes.begin (), _panes.end (), HasWindowHandle (hwndClicked));
	return iter != _panes.end ();
}

void Layout::OnFocusSwitch ()
{
	dbg << "--> Layout::OnFocusSwitch -- pane: " << std::dec << _focusRing.GetFocusId () << " has focus." << std::endl;
	std::for_each (_panes.begin (), _panes.end (), FocusSwitch (_focusRing.GetFocusId ()));
	RefreshToolBars ();
	dbg << "<-- Layout::OnFocusSwitch" << std::endl;
}

void Layout::OnClose (unsigned id)
{
	// Main pane cannot be closed
	if (_panes.size () == 1)
		return;

	Assert (_panes.size () <= 3);
	for (unsigned i = 1; i < _panes.size (); ++i)
	{
		if (_panes [i]->GetId () == id)
		{
			// Found pane to be closed
			_focusRing.RemoveId (id);
			_panes [i]->DeActivate ();

			if (i == 1)
				_hiddenHSplitter = true;
			else
				_hiddenVSplitter = true;

			Arrange ();
			break;
		}
	}
}

void Layout::OnItemChange ()
{
	RefreshToolBars ();
}

WidgetBrowser const * Layout::GetCurBrowser () const
{
	Pane const * pane = GetDragSourcePane ();
	if (pane == 0)
		pane = GetFocusPane ();
	return pane->GetBrowser ();
}

WidgetBrowser const * Layout::GetBrowser (Table::Id tableId) const
{
	WidgetBrowser const * browser = GetCurBrowser ();
	Assert (browser != 0);
	if (browser->GetTableId () == tableId)
		return browser;

	auto_vector<Pane>::const_iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualTableId (tableId));
	if (iter != _panes.end ())
		return (*iter)->GetBrowser ();

	return 0;
}

WidgetBrowser * Layout::GetCurBrowser ()
{
	Pane * pane = GetDragSourcePane ();
	if (pane == 0)
		pane = GetFocusPane ();
	return pane->GetBrowser ();
}

WidgetBrowser const * Layout::GetMainBrowser () const
{
	return _panes[0]->GetBrowser ();
}

WidgetBrowser * Layout::GetBrowser (Table::Id tableId)
{
	WidgetBrowser * browser = GetCurBrowser ();
	Assert (browser != 0);
	if (browser->GetTableId () == tableId)
		return browser;

	auto_vector<Pane>::const_iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualTableId (tableId));
	if (iter != _panes.end ())
		return (*iter)->GetBrowser ();

	return 0;
}

RecordSet const * Layout::GetRecordSet (Table::Id tableId) const
{
	RecordSet const * recordSet = GetCurRecordSet ();
	Assert (recordSet != 0);
	if (recordSet->GetTableId () == tableId)
		return recordSet;

	auto_vector<Pane>::const_iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualTableId (tableId));
	Assume (iter != _panes.end (), Table::GetName (tableId));
	Pane const * pane = *iter;
	return pane->GetRecordSet ();
}

RecordSet const * Layout::GetCurRecordSet () const
{
	return GetCurBrowser()->GetRecordSet();
}

RecordSet const * Layout::GetMainRecordSet() const
{
	return _panes[0]->GetRecordSet ();
}

Pane * Layout::GetFocusPane ()
{
	auto_vector<Pane>::iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualId (_focusRing.GetFocusId ()));
	if (iter != _panes.end ())
		return *iter;

	return 0;
}

Pane const * Layout::GetFocusPane () const
{
	auto_vector<Pane>::const_iterator iter =
		std::find_if (_panes.begin (), _panes.end (), IsEqualId (_focusRing.GetFocusId ()));
	Assert (iter != _panes.end ());
	return *iter;
}

void Layout::PositionHorizontalSplitter (Win::Rect const & rect,
										 Win::Rect & upperPane,
										 Win::Rect & lowerPane)
{
	if (_hiddenHSplitter)
	{
		upperPane = rect;
		_hSplitter.Hide ();
	}
	else
	{
		unsigned ratio = _preferences.GetHRatio ();
		Assert (20 <= ratio && ratio <= 100);
		int ySplit = rect.Height () * ratio / 100;
		if (ySplit == rect.Height ())
			ySplit -= SPLITTER_PIXEL_WIDTH;
		upperPane = Win::Rect (rect.Left (),
							   rect.Top (),
							   rect.Right (),
							   ySplit); // constructor not in terms of width/height
		lowerPane = Win::Rect (rect.Left (),
							   upperPane.Bottom () + SPLITTER_PIXEL_WIDTH,
							   rect.Right (),
							   rect.Bottom ());
		// Horizontal splitter below upper pane
		if (lowerPane.Height () < 3 * SPLITTER_PIXEL_WIDTH)
		{
			lowerPane.top = upperPane.Bottom ();
			_hSplitter.Hide ();
		}
		else
		{
			Win::Rect hSplitterRect (upperPane.Left (),
									 upperPane.Bottom (),
									 upperPane.Right (),
									 upperPane.Bottom () + SPLITTER_PIXEL_WIDTH);
			_hSplitter.Move (hSplitterRect);
			_hSplitter.Show ();
		}
	}
}

void Layout::PositionVerticalSplitter (Win::Rect const & rect,
									   Win::Rect & leftPane,
									   Win::Rect & rightPane)
{
	if (_hiddenVSplitter)
	{
		rightPane = rect;
		_vSplitter.Hide ();
	}
	else
	{
		unsigned ratio = _preferences.GetVRatio ();
		Assert (3 <= ratio && ratio <= 100);
		int xSplit = rect.Width () * ratio / 100;
		if (xSplit == rect.Width ())
			xSplit -= SPLITTER_PIXEL_WIDTH;

		leftPane = Win::Rect (rect.Left (),
							  rect.Top (),
							  xSplit,
							  rect.Bottom ()); // constructor not in terms of width/height
		rightPane = Win::Rect (leftPane.Right () + SPLITTER_PIXEL_WIDTH,
							   rect.Top (),
							   rect.Right (),
							   rect.Bottom ());
		// Vertical splitter at right edge of the left pane
		Win::Rect vSplitterRect (leftPane.Right (),
								 leftPane.Top (),
								 leftPane.Right () + SPLITTER_PIXEL_WIDTH,
								 leftPane.Bottom ());
		_vSplitter.Move (vSplitterRect);
		_vSplitter.Show ();
	}
}
