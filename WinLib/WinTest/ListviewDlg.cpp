//---------------------------
// (c) Reliable Software 2007
//---------------------------
#include "precompiled.h"
#include "ListviewDlg.h"
#include "OutSink.h"
#include "resource.h"

// ListView embedded in the dialog sends notifications to ListHandler

ListHandler::ListHandler (ListingCtrlHandler & ctrl)
	: Notify::ListViewHandler (IDC_LIST_CONTENT),
	  _ctrl (ctrl)
{}

bool ListHandler::OnDblClick () throw ()
{
	return _ctrl.OnDoubleClick ();
}

bool ListHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.GainedSelection ())
		_ctrl.Select (state.Idx ());
	return true;
}

// Control Handler of the whole dialog

ListingCtrlHandler::ListingCtrlHandler (TableData & tableData)
	: Dialog::ControlHandler (IDD_LISTING),
#pragma warning (disable:4355)
	  _notifyHandler (*this),
#pragma warning (default:4355)
	  _tableData (tableData)
{}

// Give Window Procedure access to ListView notification handler 
Notify::Handler * ListingCtrlHandler::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_notifyHandler.IsHandlerFor (idFrom))
		return &_notifyHandler;
	else
		return 0;
}

bool ListingCtrlHandler::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_content.Init (dlgWin, IDC_LIST_CONTENT);
	_itemName.Init (dlgWin, IDC_EDIT);
	_content.AddProportionalColumn (30, "Item");
	_content.AddProportionalColumn (20, "State");

	for (TableData::PairList::const_iterator it = _tableData.begin (); 
		it != _tableData.end (); 
		++it)
	{
		Win::ListView::Item item;
		item.SetText (it->first.c_str ());
		int row = _content.AppendItem (item);
		_content.AddSubItem (it->second.c_str (), row, 1);
	}
	return true;
}

bool ListingCtrlHandler::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Here one would process messages from buttons or other controls
	return false;
}

// Called when the user clicks the OK button
bool ListingCtrlHandler::OnApply () throw ()
{
	std::string itemName = _itemName.GetString ();
	if (itemName.empty ())
	{
		TheOutput.Display ("Please, select an item");
		return false;
	}

	_tableData.SetSelectedItem (itemName);
	return Dialog::ControlHandler::OnApply ();
}

// Called from ListView notification handler
void ListingCtrlHandler::Select (int itemIdx)
{
	std::string itemText = _content.RetrieveItemText (itemIdx);
	_itemName.SetString (itemText);
}

bool ListingCtrlHandler::OnDoubleClick ()
{
	int itemIdx = _content.GetFirstSelected ();
	std::string itemText = _content.RetrieveItemText (itemIdx);
	_itemName.SetString (itemText);
	OnApply ();
	return true;
}

