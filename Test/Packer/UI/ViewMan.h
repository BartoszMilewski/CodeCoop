#if !defined VIEWMAN_H
#define VIEWMAN_H
//---------------------------------------------
// (c) Reliable Software 2001
//---------------------------------------------

#include <Win/Win.h>
#include <Sys/WheelMouse.h>
#include <Ctrl/Menu.h>
#include <Ctrl/ProgressBar.h>
#include <Ctrl/StatusBar.h>

class ViewManager
{
public:
    ViewManager (Win::Dow::Handle win, Cmd::Vector & cmdVector);
	// ViewManager
	void AttachMenu2Window (Win::Dow::Handle hwnd) { _menu.AttachToWindow (hwnd); }
    void Size (int cx, int cy);
    void SetFocus ();
	void SetStatusText (char const * text) { _statusBar.SetText (text); }
	void UseSystemSettings ();
	void ShowProgressBar (int mini, int maxi, int step);
	void HideProgressBar ();

private:
	// Graphical elements
	Win::Dow::Handle	_win;		// Top window
	Menu::DropDown		_menu;
	Win::StatusBar		_statusBar;
	Win::ProgressBar	_progressBar;
};

#endif
 
