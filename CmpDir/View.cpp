//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------

#include "precompiled.h"
#include "View.h"
#include "ToolBar.h"

#include "Resource/resource.h"

#include <Win/Metrics.h>

View::View (Win::Dow::Handle winParent, CmdVector const & cmdVector, Cmd::Executor & executor)
	: _toolBar (winParent, ID_TOOLBAR, IDB_BUTTONS, Tool::DefaultButtonWidth, cmdVector, Tool::TheLayout),
	  _list (winParent, ID_LIST),
	  _merger (_mergeResult),
	  _buttonCtrl (ID_TOOLBAR, executor)
{
	NonClientMetrics metrics;
	_toolBar.SetAllButtons ();
	_buttonCtrl.AddButtonIds (_toolBar.begin (), _toolBar.end ());
	_toolBar.Refresh ();
	Win::InfoDisplayMaker displayMaker (winParent, ID_TOOLBARTEXT);
	_caption.Reset (displayMaker.Create (_toolBar));
	_caption.SetFont (metrics.GetMenuFont ());
	_toolBar.AddWindow (_caption, winParent);

	Tab::Maker tabMaker (winParent, ID_TABS);
	tabMaker.Style () << Win::Style::Ex::ClientEdge;
	_tabs.Reset (tabMaker.Create ());
	_tabs.SetFont (metrics.GetMenuFont ());
	_tabs.AddTab (TAB_NEW, "New");
	_tabs.AddTab (TAB_OLD, "Old");
	_tabs.AddTab (TAB_DIFF, "Diff");
	// Start at diff tab
	_curTab = TAB_DIFF;

	Win::StatusBarMaker statusMaker (winParent, ID_STATUSBAR);
	statusMaker.Style () << Win::Style::Ex::ClientEdge;
	_statusBar.Reset (statusMaker.Create ());
	_statusBar.AddPart (50);
}

Control::Handler * View::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_buttonCtrl.IsCmdButton (idFrom))
		return &_buttonCtrl;
	return 0;
}

void View::Size (int width, int height)
{
	// TABS
	// Get display area where the toolbar will be shown
	Win::Rect tabRect (0, 0, width, height); // full client area rectangle
	_tabs.GetDisplayArea (tabRect); // rectangle, which we can use
	int tabsHeight = height - tabRect.Height (); 

	// TOOLBAR
	// First let it position itself at the top of client rectangle
	_toolBar.AutoSize ();
	Win::ClientRect toolRect (_toolBar); // toolbar rectangle
	int toolHeight = toolRect.Height () + 1;
	// Shrink tabs window to allow only toolbar in its display area
	_tabs.ReSize  (0, 0, width, tabsHeight + toolHeight);
	int y = tabsHeight;
	// Now shift toolbar down to tabs' display area
	toolRect.ShiftTo (0, y);
	_toolBar.Move (toolRect.left, toolRect.top, toolRect.Width (), toolRect.Height ());

	// Toolbar caption
	int x = _toolBar.GetButtonsEndX () + 8;
	int textWidth = width - x;
	int textLength, textHeight;
	_caption.GetTextSize ("Sample", textLength, textHeight);
	int captionY = (toolHeight - textHeight) / 2;
	_caption.ReSize (x, captionY, textWidth, textHeight);

	// LISTVIEW
	y += toolHeight;
	int listHeight = height - y - _statusBar.Height ();

	_list.Size (0, y, width, listHeight);
	// Status bar
	y += listHeight;
	int statusHeight = _statusBar.Height ();
	_statusBar.ReSize (0, y, width, statusHeight);

}

void View::DisplayStatus (char const * status)
{
	_statusBar.SetText (status);
}

void View::Clear (bool isTwoCol) 
{
    _tabs.SetCurSelection (_curTab);
	_list.Clear (isTwoCol);
	_toolBar.Refresh ();
}

// Merge the chunks with current ListView display
void View::InitMergeSources (Data::ChunkIter begin, Data::ChunkIter end)
{
	_merger.Clear ();
	_merger.AddSources (begin, end);
}

void View::Merge ()
{
	if (!_merger.Start ())
		return;

	_merger.Do ();
	Data::ChunkHolder data = _mergeResult.GetResult ();
	Data::Chunk::const_iterator it = data->begin ();
	FileViewSequencer seq (_list);
	while (!seq.AtEnd ())
	{
		Data::Item const & viewItem = seq.GetItem ();
		while (it != data->end () && *it < viewItem)
		{
			seq.Insert (*it);
			seq.Advance ();
			++it;
		}
		// Are they equal?
		if (it != data->end () && !(viewItem < *it))
		{
			Data::Item item (*it);

			if (item.Merge (viewItem))
			{
				seq.Set (item);
			}
			++it;
		}
		seq.Advance ();
	}
	// append the rest
	while (it != data->end ())
	{
		_list.AppendItem (*it);
		++it;
	}
}
