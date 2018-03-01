#if !defined UIMANAGER_H
#define UIMANAGER_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "HierarchyView.h"
#include "TableBrowser.h"
#include "TreeBrowser.h"
#include "SelectionMan.h"
#include "DisplayMan.h"
#include "Observer.h"
#include "Layout.h"
#include "Controllers.h"
#include "InstrumentBar.h"
#include "ScriptInstrumentBar.h"
#include "MergeInstrumentBar.h"
#include "FeedbackBar.h"
#include "PreferencesStorage.h"

#include <Win/Win.h>
#include <Win/Geom.h>
#include <Sys/WheelMouse.h>
#include <Ctrl/Menu.h>

#include <iosfwd>
#include <auto_vector.h>

class FileFilter;
class PathSequencer;
class Catalog;
class NocaseSet;
class NonClientMetrics;
namespace Cmd { class Executor; }
namespace Win { class Rect; }

class UiManager: public SelectionManager, public DisplayManager, public Observer
{
	static unsigned const TREE_CTRL_ID = 20;
	static unsigned const TABLE_CTRL_ID = 21;
	static unsigned const TABLE_CTRL_ID2 = 22;
	static unsigned const BRANCH_CTRL_ID = 23;

public:
    UiManager (Win::Dow::Handle win, 
			   TableProvider & tableProvider, 
			   Cmd::Vector & cmdVector,
			   Cmd::Executor & executor);
	~UiManager ();

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, 
										unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, 
										  unsigned idFrom) throw ();
	FeedbackManager * GetFeedback ();
	void DisableUI ();
	void EnableUI ();

	// DisplayManager
    void SelectPage (ViewPage page);
	void SwitchPage (bool isForward);
	void ClosePage (ViewPage page);
    void Refresh (ViewPage page);
	void RefreshPane (ViewPage page, Table::Id id);
	bool IsEmptyPage (ViewPage page) const;
	void SetFileFilter (ViewPage page, std::unique_ptr<FileFilter> newFilter);
	// hierarchy operations
	void GoUp ();
	void GoDown (std::string const & name);
	void GotoRoot ();
	void GoTo(Vpath const & vpath);
	void Navigate (std::string const & target, int scrollPos);

	void RefreshAll ();
	void Rebuild (bool isWiki);
    void ClearAll (bool forGood);

    ViewPage GetCurrentPage () const { return _curPage; }
	Table::Id GetCurrentTableId () const;
	Table::Id GetMainTableId () const;
	Restriction const & GetPresentationRestriction () const;
	void GetScrollBookmarks (std::vector<Bookmark> & bookmakrs) const;
	void SetRestrictionFlag (std::string const & flagName, bool flagValue);
	void SetExtensionFilter (NocaseSet const & newFilter);
	void SetScrollBookmarks (std::vector<Bookmark> const & bookmakrs);
	void SetInterestingItems (GidSet const & itemIds);
	bool CanShowHierarchy (bool & isVisible) const;
	void ToggleHierarchyView ();
	bool CanShowDetails (bool & isVisible) const;
	void ToggleDetailsView ();

	// SelectionManager
	RecordSet const * GetRecordSet(Table::Id tableId) const;
	RecordSet const * GetCurRecordSet() const;
	RecordSet const * GetMainRecordSet() const;
	std::string GetInputText () const;
	void DumpRecordSet (std::ostream & out, Table::Id tableId, bool allRows);
	void GetRows (std::vector<unsigned> & rows, Table::Id tableId) const;
	void GetSelectedRows (std::vector<unsigned> & rows) const;
	void GetSelectedRows (std::vector<unsigned> & rows, Table::Id tableId) const;
	void SetSelection (PathSequencer & sequencer) {}
	void SetSelection (GidList const & ids) {}
	void SelectAll ();
	void SelectIds (GidList const & ids, Table::Id tableId);
	void SelectItems (std::vector<unsigned> const & items, Table::Id tableId);
	void SetRange (History::Range const & range);
	void VerifyRecordSet () const;
	void Clear () {}
	unsigned int SelCount () const;
	bool IsDefaultSelection () const;
	bool FindIfSome (std::function<bool(long, long)> predicate) const ;
	bool FindIfSelected (std::function<bool(long, long)> predicate) const;
	bool FindIfSelected (Table::Id tableId, std::function<bool(long, long)> predicate) const;
	bool FindIfSelected (GidPredicate const & predicate) const;
	void BeginDrag (Win::Dow::Handle win, unsigned id)
	{
		_curLayout->BeginDrag(win, id);
	}
	void EndDrag()
	{
		_curLayout->EndDrag();
	}	

	// UiManager
	void AttachMenu2Window (Win::Dow::Handle hwnd) { _menu.AttachToWindow (hwnd); }
	void SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows);
	void RefreshPopup (Menu::Handle menu, int barItemNo);
	bool DisplayContextMenu (Win::Dow::Handle hwndCmd, Win::Dow::Handle hwndClicked, int x, int y);

	void TabChanged ();
	void OnStartup ();
    void OnSize (int cx, int cy);
    void OnFocus ();
	void OnDestroy ();
	void RefreshToolBars (bool force = false);
	void LayoutChange ();

	void NonClientMetricsChanged ();
    void InPlaceEdit (int row);
    void BeginNewFolderEdit ();
    void AbortNewFolderEdit ();
	void SelectItem (int firstChar);
	
	void MoveVSplitter (int value, unsigned splitterId);
	void MoveHSplitter (int value, unsigned splitterId);
	void UpdateBrowseWindow (TableBrowser *browse);
	void RefreshPageTabs ();
		
	// Observer interface
	void UpdateAll ();

private:
	void RebuildIfNecessary (ViewPage newPage);
	void ShowCurrentPage ();
	Layout & GetLayout (ViewPage page);
	Layout const & GetLayout (ViewPage page) const;
    void DisplayCaption (bool hasOpenButton);
	void DisplayDropDown (std::string const & prefix);
	void UseSystemSettings ();

	// Construction
	std::unique_ptr<Layout> CreateLayout (Tool::BarLayout instrumentBarLayout,
										std::string const & prefsBranchName);
	std::unique_ptr<Pane::Bar> CreateFocusBar (unsigned barId,
											 unsigned associatedId,
											 std::string const & title,
											 Focus::Ring & focusRing);
	void DestroyPageLayout (ViewPage page);

	void BuildBrowsingPageLayout ();
	void BuildCheckInPageLayout ();
	void BuildMailboxPageLayout ();
	void BuildSynchPageLayout ();
	void BuildHistoryPageLayout ();
	void BuildProjectPageLayout ();
	void BuildProjectMergePageLayout ();
	void BuildBrowserPageLayout ();

private:
	// Connection to the model
	TableProvider &		_tableProvider;
	Cmd::Executor &		_executor;

	// Graphical elements
	Win::Dow::Handle	_topWin;
	Menu::DropDown		_menu;
	InstrumentBar		_instrumentBar;	// Main tool bar plus associated controls
	ScriptInstrumentBar	_scriptBar;		// Script details tool bar plus associated controls
	MergeInstrumentBar	_mergeBar;		// Merge details tool bar plus associated controls
	FeedbackBar			_feedbackBar;	// Status bar plus associated controls
	FocusBar::Use		_focusBarUser;

	// UI preferences
	Preferences::TopWinPlacement	_placement;

	int 				_cx; // cached total width of view
	int 				_cy; // cached total height of view
	Win::Rect			_layoutRect;
	
	// Controllers
	TabController		_tabCtrl;
	Tool::DynamicRebar::TipHandler	_toolTipHandler;

	// Layouts
	auto_vector<Layout>				_layouts;
	std::map<ViewPage, Layout *>	_layoutMap;

	// Bookeeping
    ViewPage			_curPage;
	Layout *			_curLayout;
};

#endif
 
