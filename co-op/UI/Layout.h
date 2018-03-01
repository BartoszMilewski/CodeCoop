#if !defined (LAYOUT_H)
#define LAYOUT_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "PreferencesStorage.h"
#include "ListObserver.h"
#include "DisplayPane.h"
#include "ButtonTable.h"
#include "Table.h"

#include <Ctrl/Splitter.h>
#include <Ctrl/Focus.h>
#include <Ctrl/FocusBarWin.h>

#include <NamedBool.h>
#include <auto_vector.h>

class FeedbackManager;
class InstrumentBar;
class Observer;

namespace History
{
	class Range;
}
namespace Win
{
	class Rect;
}
namespace FocusBar
{
	class Ctrl;
}
namespace Cmd
{
	class Executor;
}
namespace Column
{
	struct Info;
}

class Layout : public Focus::Sink, public ListObserver
{
	class Preferences : public ::Preferences::Storage
	{
	public:
		Preferences (::Preferences::Storage const & root, std::string const & nodeName);
		~Preferences ();

		void Verify ();

		unsigned GetHRatio () const { return _hSplitRatio; }
		unsigned GetVRatio () const { return _vSplitRatio; }
		bool IsHiddenVSplitter () const { return _options.IsOn ("HiddenVSplitter"); }
		bool IsHiddenHSplitter () const { return _options.IsOn ("HiddenHSplitter"); }

		void SetHRatio (unsigned ratio);
		void SetVRatio (unsigned ratio);
		void SetOptions (bool hiddenVSplitter, bool hiddenHSplitter);

	private:
		static char const * _hRatioValueName;
		static char const * _vRatioValueName;
		static char const * _optionsValueName;

	private:
		unsigned long	_vSplitRatio;	// Last remembered vertical split ratio, cannot be 0 or 100
		unsigned long	_hSplitRatio;	// Last remembered horizontal split ratio, cannot be 0 or 100
		NamedBool		_options;
	};

public:
	// specifies the control that has focus
	enum { IdFocus = -1 };
	enum DragDropBits { DragBit, DropBit };
	typedef BitSet<DragDropBits> DragDropSupport;

public:
	Layout (Win::Dow::Handle parentWin,
			InstrumentBar & instrumentBar,
			Tool::BarLayout instrumentBarLayoutId,
			Win::Rect const & layoutRect,
			::Preferences::Storage const & root,
			std::string const & prefBranchName);

	// Display pane construction
	TablePane * CreateTablePane (unsigned paneId,
								 std::string const & paneName,
								 Table::Id tableId,
								 Column::Info const * columnInfo,
								 TableProvider & tableProvider,
								 Cmd::Executor & executor,
								 DragDropSupport isDragDrop);
	TablePane * CreateRangeTablePane (unsigned paneId,
									  std::string const & paneName,
									  Table::Id tableId,
									  Column::Info const * columnInfo,
									  TableProvider & tableProvider,
									  Cmd::Executor & executor,
									  bool isSingleSelection);
	TreePane * CreateHierarchyPane (unsigned paneId,
								 Table::Id tableId,
								 TableProvider & tableProvider,
								 Cmd::Executor & executor);

	BrowserPane * CreateBrowserPane (unsigned paneId,
								 Table::Id tableId,
								 TableProvider & tableProvider,
								 Cmd::Executor & executor);

	Focus::Ring & GetFocusRing () { return _focusRing; }
	void BeginDrag(Win::Dow::Handle win, unsigned id);
	void EndDrag();
	Pane * GetDragSourcePane() { return _dragSourcePane; }
	Pane const * GetDragSourcePane() const { return _dragSourcePane; }

	void AttachObserver (Observer * observer);
	void DetachObserver (Observer * observer);

	// Main pane manipulation
	Table::Id GetMainTableId () const;
	unsigned GetMainPaneId () const;
	Restriction const & GetMainTablePresentationRestriction () const;
	void GetMainTableScrollBookmarks (std::vector<Bookmark> & bookmarks) const;
	virtual bool HasTable(Table::Id tableId) const;
	void VerifyMainTableRecordSet () const;
	bool IsMainTableEmpty () const;
	DegreeOfInterest HowInterestingIsMainTable () const;
	void SetMainTableRestrictionFlag (std::string const & name, bool val);
	void SetMainTableExtensionFilter (NocaseSet const & newFilter);
	void SetMainTableScrollBookmarks (std::vector<Bookmark> const & bookmarks);
	void SetMainTableInterestingItems (GidSet const & itemIds);
	void SetMainTableFileFilter (std::unique_ptr<FileFilter> newFilter);
	void SetMainTableRange (History::Range const & range);

	// Direct item editing (label editing)
	void InPlaceEdit (int row);
	void NewItemCreation ();		// New item is added to the one of layout display panes
	void AbortNewItemCreation ();

	void Activate (FeedbackManager & feedback);
	void DeActivate ();
	void Invalidate ();
	void Invalidate (Table::Id tableId);
	void Clear (bool forGood);

	WidgetBrowser const * GetBrowser (Table::Id id) const;
	WidgetBrowser * GetBrowser (Table::Id id);
	WidgetBrowser const * GetCurBrowser () const;
	WidgetBrowser * GetCurBrowser ();
	WidgetBrowser const * GetMainBrowser () const;

	RecordSet const * GetRecordSet (Table::Id tableId) const;
	RecordSet const * GetCurRecordSet() const;
	RecordSet const * GetMainRecordSet () const;
	void Arrange ();
	void RefreshToolBars (bool force = false);

	// Tree operations
	void GoUp ();
	void GoDown (std::string const & name);
	void GoToRoot ();
	void GoTo(Vpath const & vpath);
	bool CanShowHierarchy (bool & isVisible) const;
	void ToggleHierarchyView ();
	bool CanShowDetails (bool & isVisible) const;
	void ToggleDetailsView (FeedbackManager & feedback);
	void MoveVSplitter (int pixelPos);
	void MoveHSplitter (int pixelPos);

	void Navigate (std::string const & target, int scrollPos);

	// Notifications
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, 
										unsigned idFrom) throw ();

	// Context menu
	bool HasWindow (Win::Dow::Handle hwndClicked);

	// Focus Sink
	void OnFocusSwitch ();
	void OnClose (unsigned id);

	// List observer
	void OnItemChange ();

private:
	void AddPane (std::unique_ptr<Pane> pane);
	Pane * GetFocusPane ();
	Pane const * GetFocusPane () const;
	void PositionHorizontalSplitter (Win::Rect const & rect,
									 Win::Rect & upperPane,
									 Win::Rect & lowerPane);
	void PositionVerticalSplitter (Win::Rect const & rect,
								   Win::Rect & leftPane,
								   Win::Rect & rightPane);

private:
	Win::Dow::Handle	_parentWin;
	Preferences			_preferences;
	InstrumentBar &		_instrumentBar;
	Tool::BarLayout		_instrumentBarLayoutId;
	Win::Rect const &	_layoutRect;
	Focus::Ring 		_focusRing;
	auto_vector<Pane>	_panes;
	Pane *				_dragSourcePane;

	static const unsigned	VSPLITTER_ID = 1;
	static const unsigned	HSPLITTER_ID = 2;
	static const unsigned	SPLITTER_PIXEL_WIDTH = 8;
	Splitter::UseVertical	_vSplitterUser;
	Splitter::UseHorizontal	_hSplitterUser;
	Win::Dow::Handle		_vSplitter;
	Win::Dow::Handle		_hSplitter;
	bool 					_hiddenVSplitter;// is the split panel currently shown?
	bool 					_hiddenHSplitter;// is the split panel currently shown?
};

#endif
