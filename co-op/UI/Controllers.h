#if !defined (CONTROLLERS_H)
#define CONTROLLERS_H
//------------------------------------
//  (c) Reliable Software, 2005 - 2008
//------------------------------------

#include "TableViewer.h"
#include "HierarchyView.h"
#include "BranchView.h"
#include "Tab.h"
#include "PopupDisplay.h"
#include "UiStrings.h"

#include <Win/Keyboard.h>
#include <Ctrl/Tree.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Tooltip.h>
#include <Ctrl/Tabs.h>
#include <Graph/Cursor.h>
#include <Ctrl/DropDown.h>
#include <Ctrl/ProgressBar.h>
#include <Ctrl/InfoDisp.h>
#include <Win/ControlHandler.h>
#include <Com/DragDrop.h>

class TreeBrowser;
class BranchBrowser;
class Layout;
class UiManager;
class ListObserver;
namespace Tool { class DynamicRebar; }
namespace Cmd { class Executor; }
namespace Focus { class Ring; }

namespace Notify
{
	class BranchHandler : public Notify::Handler
	{
	public:
		explicit BranchHandler (unsigned id) : Notify::Handler (id) {}
	private:
		bool OnNotify (NMHDR * hdr, long & result)
		{
			return false;
		}
	};
}

class BranchController: public Notify::BranchHandler
{
public:
	class BranchKbdHandler : public Keyboard::Handler
	{
	public:
		BranchKbdHandler (Focus::Ring & focusRing)
			: _focusRing (focusRing)
		{}

		bool OnTab () throw ();

	private:
		Focus::Ring &	_focusRing;
	};
public:
	BranchController (int id,
					Win::Dow::Handle win,
					Focus::Ring & focusRing);

	BranchView & GetView () { return _view; }
	void SetFocus () { _view.SetFocus (); }
	void HideView () { _view.Hide (); }
	void ShowView () { _view.Show (); }
	void MoveView (Win::Rect const & viewRect) { _view.Move (viewRect);	}

private:
	int					_id;
	Focus::Ring &		_focusRing;
	BranchKbdHandler	_kbdHandler;

	BranchView			_view;
	BranchBrowser *		_browser;
};

class HierarchyController: public Notify::TreeViewHandler, public Win::FileDropSink
{
	class HierarchyKbdHandler: public Keyboard::Handler
	{
	public:
		HierarchyKbdHandler (Focus::Ring & focusRing)
			: _focusRing (focusRing)
		{}

		bool OnTab () throw ();

	private:
		Focus::Ring &	_focusRing;
	};

public:
	HierarchyController (int id,
						 Win::Dow::Handle win,
						 Cmd::Executor & executor,
						 Focus::Ring & focusRing);

	int GetId () const { return _id; }
	HierarchyView & GetView () { return _view; }
	bool IsActive () const;
	void Activate (TreeBrowser * browser, ListObserver * listObserver)
	{
		_browser = browser;
		_listObserver = listObserver;
	}

	void SetFocus () { _view.SetFocus (); }
	void HideView () { _view.Hide (); }
	void ShowView () { _view.Show (); }
	void MoveView (Win::Rect const & viewRect) { _view.Move (viewRect);	}
	bool IsInsideView (Win::Point const & screenPt) const;
	// TreeView notifications
	bool OnClick (Win::Point pt) throw ();
	bool OnRClick (Win::Point pt) throw ();
	bool OnDblClick (Win::Point pt) throw ();
	bool OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnGetDispInfo (Tree::View::Request const & request,
				Tree::View::State const & state,
				Tree::View::Item & item) throw ();
	bool OnItemExpanding (Tree::View::Item & item,
						Tree::Notify::Action action,
						bool & allow) throw ();
	bool OnItemExpanded (Tree::View::Item & item,
						Tree::Notify::Action action) throw ();
	bool OnItemCollapsing (Tree::View::Item & item,
						Tree::Notify::Action action,
						bool & allow) throw ();
	bool OnItemCollapsed (Tree::View::Item & item,
						Tree::Notify::Action action) throw ();
	bool OnSelChanging (Tree::View::Item & itemOld, 
						Tree::View::Item & itemNew, 
						Tree::Notify::Action action) throw ();
	bool OnSelChanged (Tree::View::Item & itemOld, 
						Tree::View::Item & itemNew, 
						Tree::Notify::Action action) throw ();
	Keyboard::Handler * GetKeyboardHandler () throw ()
	{
		return &_kbdHandler;
	}

	//  Win::FileDropSink
	void OnDragEnter (Win::Point screenDropPoint);
	void OnDragLeave ();
	void OnDragOver (Win::Point screenDropPoint);
	void OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point screenDropPoint);

private:
	Tree::NodeHandle GetHitItem (Win::Point screenDropPoint);
	void SetDropTargetItem (Win::Point screenDropPoint);
	void ClearDropTargetItem ();
	void ChangeCurFolder (Tree::NodeHandle folderNode);

private:
	int					_id;
	Cmd::Executor &		_executor;
	Focus::Ring &		_focusRing;
	HierarchyKbdHandler	_kbdHandler;

	HierarchyView 		_view;
	TreeBrowser *		_browser;
	ListObserver *		_listObserver;
	Tree::NodeHandle	_previousDragOverItem;
};

class TableController: public Notify::ListViewHandler
{
	class ItemKbdHandler: public Keyboard::Handler
	{
	public:
		ItemKbdHandler (Cmd::Executor & executor, Focus::Ring & focusRing)
			: _focusRing (focusRing),
			  _executor (executor)
		{}

		bool OnTab () throw ();
		bool OnReturn () throw ();

	private:
		Focus::Ring &	_focusRing;
		Cmd::Executor & _executor;
	};

	class FontHandler
	{
	public:
		FontHandler (bool isBold, bool isItalic)
			: _isBold (isBold), _isItalic (isItalic)
		{}
		void SelectFont (Win::ListView::CustomDraw & customDraw);
	private:
		bool _isBold;
		bool _isItalic;
		Font::AutoHandle _font;
	};

public:
	TableController (int id,
					 Win::Dow::Handle win,
					 Cmd::Executor & executor,
					 Focus::Ring & focusRing,
					 bool isDragSource,
					 bool isSingleSelection);

	int GetId () const { return _id; }

	void SetFocus () { _view.SetFocus (); }
	void SetNewItemCreation (bool flag) { _newItem = flag; }
	void MoveView (Win::Rect const & viewRect) { _view.Move (viewRect); }
	bool IsInsideView (Win::Point const & screenPt) const;
	// Notify::ListViewHandler
	bool OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnDblClick () throw ();
	bool OnGetDispInfo (Win::ListView::Request const & request,
						Win::ListView::State const & state,
						Win::ListView::Item & item) throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
	bool OnBeginLabelEdit (long & result) throw ();
	bool OnEndLabelEdit (Win::ListView::Item * item, long & result) throw ();
	bool OnColumnClick (int col) throw ();
	bool OnKeyDown (int key) throw ();
	void OnCustomDraw (Win::ListView::CustomDraw & customDraw, long & result) throw ();
	Keyboard::Handler * GetKeyboardHandler () throw ()
	{
		return &_kbdHandler;
	}
	void OnBeginDrag (Win::Dow::Handle winFrom, unsigned idFrom, int itemIdx, bool isRightButtonDrag) throw ();

	TableViewer & GetView () { return _view; }
	void HideView () { _view.Hide (); }
	virtual void ShowView () { _view.Show (); }
	void Activate (TableBrowser * browser, ListObserver * listObserver)
	{
		_browser = browser;
		_listObserver = listObserver;
	}

protected:
	int				_id;
	Cmd::Executor & _executor;
	Focus::Ring &	_focusRing;
	ItemKbdHandler	_kbdHandler;
	bool			_newItem;
	bool			_isDragSource;

	TableViewer		_view;
	TableBrowser *	_browser;
	ListObserver *	_listObserver;
	FontHandler		_boldFont;
	FontHandler		_italicFont;
};

class RangeTableController : public TableController
{
public:
	RangeTableController (int id,
						  Win::Dow::Handle win,
						  Cmd::Executor & executor,
						  Focus::Ring & focusRing,
						  bool isSingleSelection)
		: TableController (id, win, executor, focusRing, false, isSingleSelection)
	{}

	// Notify::ListViewHandler
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();

	void ShowView ();
};

class FileDropTableController : public TableController, public Win::FileDropSink
{
public:
	FileDropTableController (int id,
							 Win::Dow::Handle win,
							 Cmd::Executor & executor,
							 Focus::Ring & focusRing,
							 bool isDragSource,
							 bool isSingleSelection);

	void OnDragEnter (Win::Point screenDropPoint);
	void OnDragLeave ();
	void OnDragOver (Win::Point screenDropPoint);
	void OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point screenDropPoint);

private:
	int GetHitItem (Win::Point screenDropPoint);
	void SetDropTargetItem (Win::Point screenDropPoint);
	void ClearDropTargetItem ();

private:
	int	_previousDragOverItem;
};

class TabController: public Notify::TabHandler
{
public: 
	typedef EnumeratedTabs<ViewPage> View;
public:
	TabController (Win::Dow::Handle win, UiManager * uiMan);

	// Notify::TabHandler
	bool OnSelChange () throw ();

	View & GetView () { return _view; }
	void RemoveImage (ViewPage page) { _view.RemovePageImage (page); }
	void SetImage (ViewPage page, int imgIdx) { _view.SetPageImage (page, imgIdx); }
private:
	UiManager *	_uiMan;

	PageTabs	_view;
};

class DropDownCtrl : public Control::Handler
{
public:
	DropDownCtrl (int id, Win::Dow::Handle topWin, Win::Dow::Handle canvasWin)
		: Control::Handler (id),
		  _dropDown (topWin, id, canvasWin)
	{}

	int GetId () const { return _id; }
	void MoveView (Win::Rect const & viewRect) { _dropDown.Move (viewRect); }
	void HideView () { _dropDown.Hide (); }
	void ShowView () { _dropDown.Show (); }
	void SetFont (Font::Descriptor const & font) { _dropDown.SetFont (font); }
	Win::Dow::Handle GetEditWindow () const { return _dropDown.GetEditWindow (); }
	Win::Dow::Handle GetView () const { return _dropDown; }
	unsigned GetHeight () const { return _dropDown.GetHeight (); }

	void Refresh (std::string const & items);

protected:
	Win::DropDown	_dropDown;
};

class HistoryFilterCtrl : public DropDownCtrl
{
public:
	HistoryFilterCtrl (int id,
					   Win::Dow::Handle topWin,
					   Win::Dow::Handle canvasWin,
					   Cmd::Executor & executor);

	bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

private:
	Cmd::Executor &	_executor;
};

class TargetProjectCtrl : public DropDownCtrl
{
public:
	TargetProjectCtrl (int id,
					   Win::Dow::Handle topWin,
					   Win::Dow::Handle canvasWin,
					   Cmd::Executor & executor);

	bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

private:
	Cmd::Executor &	_executor;
};

class MergeTypeCtrl : public Control::Handler
{
public:
	MergeTypeCtrl (int id,
				   Win::Dow::Handle topWin,
				   Win::Dow::Handle canvasWin,
				   Cmd::Executor & executor);

	int GetId () const { return _id; }
	void MoveView (Win::Rect const & viewRect) { _dropDownList.Move (viewRect); }
	void HideView () { _dropDownList.Hide (); }
	void ShowView () { _dropDownList.Show (); }
	void SetFont (Font::Descriptor const & font) { _dropDownList.SetFont (font); }
	Win::Dow::Handle GetEditWindow () const { return _dropDownList.GetEditWindow (); }
	Win::Dow::Handle GetView () const { return _dropDownList; }
	unsigned GetHeight () const { return _dropDownList.GetHeight (); }

	void Refresh (MergeTypeEntry const items []);
	void Select (std::string const & item);
	bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

private:
	Win::ComboBox	_dropDownList;
	Cmd::Executor &	_executor;
};

class InfoDisplayCtrl : public Control::Handler
{
public:
	InfoDisplayCtrl (int id,
					 Win::Dow::Handle topWin,
					 Win::Dow::Handle canvasWin,
					 Cmd::Executor & executor);

	bool OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception);

	Win::Dow::Handle GetWindow () const { return _display; }
	void Move (Win::Rect const & rect) { _display.Move (rect); }
	void SetFont (Font::Descriptor const & font) { _display.SetFont (font); }
	void SetText (std::string const & text) { _display.SetText (text.c_str ()); }

private:
	Win::ReadOnlyDisplay	_display;
	Cmd::Executor &			_executor;
};

#endif
