#if !defined (VIEW_H)
#define VIEW_H
//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "ListViewer.h"
#include "CmdVector.h"
#include "DataPortal.h"
#include <Ctrl/ToolBar.h>
#include <Ctrl/StatusBar.h>
#include <Ctrl/Tabs.h>
#include <Ctrl/InfoDisp.h>

// Purpose: Combine FileListView with toolbar and status bar
//          Connect toolbar to command vector
class View
{
public:
	enum { TAB_NEW, TAB_OLD, TAB_DIFF };
public:
	View (Win::Dow::Handle winParent, CmdVector const & cmdVector, Cmd::Executor & executor);
	FileListView & GetListView () { return _list; }
	Tab::Handle & GetTabView () { return _tabs; }
	Tool::Bar & GetToolBarView () { return _toolBar; }
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();

	void Size (int width, int height);
	void DisplayStatus (char const * status);
	void Clear (bool isTwoCol);
	void SetTabOld () { _curTab = TAB_OLD; }
	void SetTabNew () { _curTab = TAB_NEW; }
	void SetTabDiff () { _curTab = TAB_DIFF; }
	void SetText (std::string const & text) { _caption.SetText (text); }
	bool IsSelection () const { return _list.GetSelectedCount () != 0; }

	void InitMergeSources (Data::ChunkIter begin, Data::ChunkIter end);
	void Merge ();
	Data::Item const & GetItem (int i) const { return _list.GetItem (i); }
	bool IsCmdButton (int id) const { return _toolBar.IsCmdButton (id); }
	int GetFirstSelected () const { return _list.GetFirstSelected (); }
private:
	int					_curTab;
	Tool::Bar			_toolBar;
	Tool::ButtonCtrl	_buttonCtrl;
	Win::InfoDisplay	_caption;
	Tab::Handle			_tabs;
	FileListView		_list;
	Win::StatusBar		_statusBar;

	Data::Processor		_mergeResult;
	Data::Merger		_merger;
};

#endif
