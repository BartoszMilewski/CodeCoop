//----------------------------------
//  (c) Reliable Software, 2007-09
//----------------------------------

#include "precompiled.h"
#include "UiManager.h"
#include "MenuTable.h"

#include <Win/Metrics.h>

UiManager::UiManager (Win::Dow::Handle win, 
					  Cmd::Vector & cmdVector,
					  Cmd::Executor & executor)
	: _executor (executor),
	  _topWin (win),
	  _menu (Menu::barItems, cmdVector),
	  _preferences (_topWin, "Software\\Reliable Software\\Experiment"),
	  _cx (0),
	  _cy (0)
{
	UseSystemSettings ();
	_pane.reset(new Pane(win));
}

UiManager::~UiManager ()
{}

Notify::Handler * UiManager::GetNotifyHandler (Win::Dow::Handle winFrom, 
											   unsigned idFrom) throw ()
{
	return 0;
}

Control::Handler * UiManager::GetControlHandler (Win::Dow::Handle winFrom, 
												 unsigned idFrom) throw ()
{
	return 0;
}

void UiManager::OnSize (int cx, int cy)
{
	_cx = cx;
	_cy = cy;
	Win::Rect clientArea(0, 0, cx, cy);
	_pane->Move(clientArea);
}

void UiManager::OnFocus (Win::Dow::Handle winPrev)
{
}

void UiManager::OnDestroy ()
{
}

void UiManager::SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows)
{
	_preferences.PlaceWindow (cmdShow, multipleWindows);
}

void UiManager::RefreshPopup (Menu::Handle menu, int barItemNo)
{
	_menu.RefreshPopup (menu, barItemNo);
}

void UiManager::UseSystemSettings ()
{
	NonClientMetrics metrics;
	if (metrics.IsValid ())
	{
		//_tabCtrl.GetView ().SetFont (metrics.GetMenuFont ());
		//_feedbackBar.SetFont (metrics.GetStatusFont ());
		//_instrumentBar.SetFont (metrics.GetStatusFont ());
		//_scriptBar.SetFont (metrics.GetStatusFont ());
		//_mergeBar.SetFont (metrics.GetStatusFont ());
	}
}
