#if !defined (WIDGETBROWSER_H)
#define WIDGETBROWSER_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2007
//------------------------------------

#include "Observer.h"
#include "Table.h"
#include "RecordSet.h"
#include "SelectIter.h"

class GidPredicate;
class NamePredicate;
class FeedbackManager;

namespace History
{
	class Range;
}

class WidgetBrowser : public Observer, public Notifier
{
public:
	WidgetBrowser (TableProvider & tableProv, Table::Id tableId)
		: _tableProv (tableProv), 
          _tableId (tableId)
	{}
	virtual ~WidgetBrowser () {}

	Table::Id GetTableId () const { return _tableId; }
	RecordSet const * GetRecordSet () const { return _recordSet.get (); }
	std::string const & GetCaption () const { return _caption; }
	Restriction const & GetPresentationRestriction () const { return _restriction; }
	void ClearRestriction () { _restriction.Clear (); }
	void SetRestrictionFlag (std::string const & name, bool val = true)
	{
		_restriction.Set (name, val);
	}
	bool GetRestrictionFlag (std::string const & name)
	{
		return _restriction.IsOn (name);
	}
	void SetExtensionFilter (NocaseSet const & newFilter)
	{
		_restriction.SetExtensionFilter (newFilter);
	}
	NocaseSet const & GetExtensionFilter () const { return _restriction.GetExtensionFilter (); }
	void SetFileFilter (std::unique_ptr<FileFilter> newFilter);
	FileFilter const * GetFileFilter () const { return _restriction.GetFileFilter (); }

	bool IsEmpty () const;
	DegreeOfInterest HowInteresting () const;
	bool IsDefaultSelection () const;
	bool SupportsDetailsView () const;

	// Returns true when first time shown
	virtual bool Show (FeedbackManager & feedback) = 0;
	virtual void Hide () = 0;
	virtual void OnFocus () = 0;
	virtual void Invalidate () = 0;
	virtual void Clear (bool forGood) = 0;

	virtual void GetScrollBookmarks (std::vector<Bookmark> & bookmarks) const {}
	virtual void SetScrollBookmarks (std::vector<Bookmark> const & bookmarks) {}
	virtual void SetInterestingItems (GidSet const &  itemIds) {}
	virtual unsigned SelCount () const { return 0; }
	virtual void SetRange (History::Range const & range) {}
	virtual bool FindIfSelected (std::function<bool(long, long)> predicate) const 
		{ return false; }
	virtual bool FindIfSelected (GidPredicate const & predicate) const
		{ return false; }
	virtual bool FindIfSome (std::function<bool(long, long)> predicate) const 
		{ return false; }
	virtual void GetSelectedRows (std::vector<unsigned> & rows) const {}
	virtual void GetAllRows (std::vector<unsigned> & rows) const {}
	virtual int SelectIf (NamePredicate const & predicate) { return 0; }
	virtual void SelectAll () {}
	virtual void SelectIds (GidList const & ids) {}
	virtual void SelectItems (std::vector<unsigned> const & items) {}
	virtual void ScrollToItem (unsigned iItem) {}
	virtual void InPlaceEdit (unsigned row) {}
	virtual void BeginNewItemEdit () {}
	virtual void AbortNewItemEdit () {}

protected:
    TableProvider &				_tableProv;
	Table::Id					_tableId;
    std::unique_ptr<RecordSet>	_recordSet;
	std::string					_caption;
	Restriction					_restriction;
};

#endif
