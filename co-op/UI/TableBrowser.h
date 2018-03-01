#if !defined TABLEBROWSER_H
#define TABLEBROWSER_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "WidgetBrowser.h"
#include "TableViewer.h"
#include "RecordSet.h"
#include "SelectIter.h"
#include "Table.h"
#include "TablePreferences.h"

#include <auto_vector.h>

class TableProvider;
class FileFilter;
class NamePredicate;
class GidPredicate;
class FeedbackManager;
namespace Win
{
	class ProgressBar;
	class StatusBar;
}
namespace Column
{
	struct Info;
}

class TableBrowser : public WidgetBrowser
{
protected:
	static unsigned const InvalidValue = 0xffffffff;

protected:
	class Selection
	{
		class IsSelectedItem : public std::unary_function<SelectState, bool>
		{
		public:
			bool operator () (SelectState state) const
			{
				return state.IsSelected ();
			}
		};

		class IsRangeItem : public std::unary_function<SelectState, bool>
		{
		public:
			bool operator () (SelectState state) const
			{
				return state.IsRange ();
			}
		};

	public:
		Selection ()
			: _selCount (0),
			  _lastSelectedItem (InvalidValue),
			  _firstRangeItem (InvalidValue),
			  _lastRangeItem (InvalidValue)
		{}

		unsigned Count () const { return _selCount; }

		SelectState State (unsigned iItem) const
		{
			Assert (iItem < _state.size ());
			return _state [iItem];
		}

		unsigned GetLastSelectedItem () const { return _lastSelectedItem; }

		bool IsSelected (unsigned iItem) const
		{
			Assert (iItem < _state.size ());
			return State (iItem).IsSelected ();
		}

		bool IsRange (unsigned iItem) const
		{
			Assert (iItem < _state.size ());
			return State (iItem).IsRange ();
		}

		void AppendItem ()
		{
			SelectState notSelected;
			_state.push_back (notSelected);
		}

		void RemoveItem (unsigned iItem);
		void Init (unsigned newSize);
		void Clear ();
		void ClearRange ();
		void GetRangeLimits (unsigned & firstRangeItem, unsigned & lastRangeItem)
		{
			firstRangeItem = _firstRangeItem;
			lastRangeItem = _lastRangeItem;
		}
		void Select (unsigned iItem);
		void DeSelect (unsigned iItem);
		void SetRange (unsigned iItem, bool flag);
		unsigned FindFirstSelected (unsigned iItemStart) const;

	private:
		unsigned					_selCount;
		std::vector<SelectState>	_state;
		unsigned					_lastSelectedItem;
		unsigned					_firstRangeItem;
		unsigned					_lastRangeItem;
	};

	class ViewMemento
	{
	public:
		ViewMemento (std::string const & visibleName);
		ViewMemento (TableBrowser const & browser);

		bool RestoreState (TableBrowser & browser);

	private:
		std::vector<Bookmark>	_selectedRows;
		std::unique_ptr<Bookmark>	_lastSelectedItem;
		std::vector<Bookmark>	_scrollBookmarks;
	};

	class ViewRefresher
	{
	public:
		ViewRefresher (TableBrowser & browser)
			: _browser (browser)
		{
			if (_browser._recordSet.get () != 0)
				_oldRecordSetName = Table::GetName (_browser._recordSet->GetTableId ());
		}

		void UpdateView ();

	private:
		TableBrowser &	_browser;
		std::string		_oldRecordSetName;
	};

	friend class ViewRefresher;

public:
	TableBrowser (TableProvider & tableProv,
				  Table::Id tableId,
				  Preferences::Storage const & storage,
				  std::string const & paneName,
				  Column::Info const * columnInfo,
				  TableViewer & view);

	// Showing & painting

	bool RefreshRecordSetIfNecessary (bool isNotification, FeedbackManager * feedback = 0);
	void RebuildView ();

    // Notification messages from ListView

    int  GetRow (unsigned iItem) const { return _sortVector [iItem]; }
    void CopyField (unsigned iItem, unsigned int col, char * buf, unsigned int bufLen) const;
    virtual void GetImage (unsigned iItem, int & iImage, int & iOverlay) const;
    void SelChanged (unsigned iItem, bool wasSelected, bool isSelected);

	// Sorting

    void Resort (unsigned int newSortCol);

    // Selections

    SelectState GetState (unsigned iItem) const { return _selection.State (iItem); }
    bool IsSelected (unsigned iItem) const { return _selection.IsSelected (iItem); }
	bool IsSelectionAtEnd () const;
	void SelectIf (std::function<bool(long, long)> predicate);

	// Widget browser interface

	bool Show (FeedbackManager & feedback);
	void Hide ();
	void OnFocus ();
	void Invalidate ();
	void Clear (bool forGood);
	void GetScrollBookmarks (std::vector<Bookmark> & bookmarks) const;
	void SetScrollBookmarks (std::vector<Bookmark> const & bookmarks);
	unsigned SelCount () const { return _selection.Count (); }
	void SetRange (History::Range const & range);
	bool FindIfSelected (std::function<bool(long, long)> predicate) const;
	bool FindIfSelected (GidPredicate const & predicate) const;
	bool FindIfSome (std::function<bool(long, long)> predicate) const;
	void GetSelectedRows (std::vector<unsigned> & rows) const;
	void GetAllRows (std::vector<unsigned> & rows) const;
	int SelectIf (NamePredicate const & predicate);
	void SelectAll ()
	{
		// Selection state will be updated automatically via SelChanged
		_tableView.SelectAll ();
	}
	void SelectIds (GidList const & ids);
	void SelectItems (std::vector<unsigned> const & items);
	bool IsDefaultSelection () const;
	void ScrollToItem (unsigned iItem);
	void InPlaceEdit (unsigned row);
	void BeginNewItemEdit ();
	void AbortNewItemEdit ();

	// Observer interface

	void UpdateAll ();
	void Update (std::vector<RowChange> const & changes);
	void UpdateIfNecessary (char const * topic);


	void GetSelectionBookmarks (std::vector<Bookmark> & bookmakrs,
								std::unique_ptr<Bookmark> & lastSelectionBookmark) const;
	// Returns true when selection restored
	bool SetSelectionBookmarks (std::vector<Bookmark> const & bookmakrs,
								Bookmark const * lastSelectionBookmark);

protected:
    void Select (unsigned iItem);
    void DeSelect (unsigned iItem);

	unsigned FindSelection (unsigned iStart = 0) const;

	void InitSort ();
    virtual bool Sort ();
	bool NeedSort (unsigned row);
	virtual bool IsUpsideDown () const { return false; }
	unsigned RowToViewIndex (unsigned row) const;
	void RepopulateView ();
	void SaveState ();
	void RestoreState ();
	virtual void OnFirstRefresh () {}

protected:
	TablePreferences	_preferences;

    TableViewer &		_tableView;
	bool				_isVisible;			// True - browser data is visible on the screen
	bool				_viewNeedsUpdate;	// True - view is different from the record set

    std::vector<unsigned> _sortVector;
	Selection			_selection;
	std::unique_ptr<ViewMemento>	_memento;
};

class RangeTableBrowser : public TableBrowser
{
public:
	RangeTableBrowser (TableProvider & tableProv,
					   Table::Id tableId,
					   Preferences::Storage const & storage,
					   std::string const & paneName,
					   Column::Info const * columnInfo,
					   TableViewer & view)
		: TableBrowser (tableProv, tableId, storage, paneName, columnInfo, view)
	{}

    void GetImage (unsigned iItem, int & iImage, int & iOverlay) const;
	void SetInterestingItems (GidSet const & itemIds)
	{
		_restriction.SetInterestingItems (itemIds);
	}

protected:
	// Special sorting in the range table
	bool Sort ();
	bool IsUpsideDown () const;
	void OnFirstRefresh ();
};

#endif
