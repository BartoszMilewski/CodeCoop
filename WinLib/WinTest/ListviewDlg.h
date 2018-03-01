#if !defined (LISTVIEWDLG_H)
#define LISTVIEWDLG_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------
#include <Ctrl/ListView.h>
#include <Ctrl/Edit.h>
#include <Win/Dialog.h>

// Data source for the dialog. Contains a list of table tuples
// On exit, the string _selection is set to user selection

class TableData
{
public:
	// Vectors of pairs of strings: (item name, item state)
	typedef std::vector<std::pair<std::string, std::string> > PairList;
public:
	void AddRow (std::string const & first, std::string const & second)
	{
		_list.push_back (std::make_pair (first, second));
	}
	PairList::const_iterator begin () const { return _list.begin (); }
	PairList::const_iterator end () const { return _list.end (); }
	void SetSelectedItem (std::string const & name)
	{
		_selection = name;
	}
	std::string const & GetSelection () const
	{
		return _selection;
	}
private:
	 PairList _list;
	 std::string _selection;
};

class ListingCtrlHandler;

// Handler for ListView notifications
class ListHandler : public Notify::ListViewHandler
{
public:
	explicit ListHandler (ListingCtrlHandler & ctrl);
	bool OnDblClick () throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
private:
	ListingCtrlHandler & _ctrl;
};

// Control Handler for the whole dialog
class ListingCtrlHandler : public Dialog::ControlHandler
{
	friend class ListHandler; // ListView handler calls private methods Select and OnDoubleClick
public:
	ListingCtrlHandler (TableData & dlgData);
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
private:
	// Called by ListView handler
	void Select (int itemIdx);
	bool OnDoubleClick ();
private:
	// Controls
	Win::ReportListing	_content;
	Win::Edit			_itemName;

	TableData &			_tableData;
	ListHandler			_notifyHandler;
};



#endif
