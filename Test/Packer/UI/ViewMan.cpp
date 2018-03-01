//---------------------------------------------
// (c) Reliable Software 2001 -- 2003
//---------------------------------------------

#include "precompiled.h"
#include "ViewMan.h"
#include "MenuTable.h"
#include "Global.h"
#include "resource.h"

#include <Win/Metrics.h>
#include <string>

ViewManager::ViewManager (Win::Dow::Handle win, Cmd::Vector & cmdVector)
:	_win (win),
	_statusBar (0),
	_progressBar (0),
	_menu (Menu::barItems, cmdVector)
{
	Win::StatusBarMaker statusMaker (_win, 1001);
	_statusBar.Reset (statusMaker.Create ());
	_statusBar.AddPart (40);
	_statusBar.AddPart (60);
	UseSystemSettings ();
}

void ViewManager::ShowProgressBar (int mini, int maxi, int step)
{
	_progressBar.Show ();
	_progressBar.SetRange (mini, maxi, step);
}

void ViewManager::HideProgressBar ()
{
	_progressBar.Hide ();
	_statusBar.SetText ("Ready");
}

void ViewManager::Size (int cx, int cy)
{
#if 0
	// Tabs
	int y = 0;
	int height = _viewTabs.Height ();
	_viewTabs.ReSize  (0, y, cx, height);
	// Toolbar
	y += height;
	height = _toolBar.Height ();
	_toolBar.ReSize   (0, y, cx, height);
	// Resize tool bar caption
	int x = _toolBar.GetRightExtent ();
	int width = cx - x;
	_caption.ReSize (x, 0, width, height);
	// Resize tool bar drop down list
	// The drop down list has height equal 1/2 main window height
	_dropDown.ReSize (x, 0, width, cy/2);
	// ListView
	y += height;
	height = cy - y - _statusBar.Height ();
	_view.ReSize (0, y, cx, height);
	// Status bar
	y += height;
	height = _statusBar.Height ();
	_statusBar.ReSize (0, y, cx, height);
	RECT rect;
	_statusBar.GetPartRect (rect, 1);
	// Progress bar if visible obscures second status bar part
	_progressBar.ReSize (rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
#endif
}

void ViewManager::SetFocus ()
{
}

void ViewManager::UseSystemSettings ()
{
	NonClientMetrics metrics;
	if (metrics.IsValid ())
	{
		_statusBar.SetFont (metrics.GetStatusFont ());
	}
}
