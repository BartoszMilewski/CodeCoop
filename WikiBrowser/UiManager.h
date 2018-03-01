#if !defined (UIMANAGER_H)
#define UIMANAGER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "PreferencesStorage.h"

#include <Ctrl/Menu.h>
#include <Win/Geom.h>

namespace Cmd { class Executor; }
namespace Notify { class Handler; }
namespace Control { class Handler; }

class UiManager
{
public:
    UiManager (Win::Dow::Handle win, 
			   Cmd::Vector & cmdVector,
			   Cmd::Executor & executor);
	~UiManager ();

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, 
										unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, 
										  unsigned idFrom) throw ();

    void OnSize (int cx, int cy);
	void OnFocus (Win::Dow::Handle winPrev);
	void OnDestroy ();

	void AttachMenu2Window (Win::Dow::Handle hwnd) { _menu.AttachToWindow (hwnd); }
	void SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows);
	void RefreshPopup (Menu::Handle menu, int barItemNo);

private:
	void UseSystemSettings ();

private:
	Cmd::Executor &		_executor;

	// Graphical elements
	Win::Dow::Handle	_topWin;
	Menu::DropDown		_menu;

	// UI preferences
	Preferences::TopWinPlacement	_preferences;

	int 				_cx; // cached total width of view
	int 				_cy; // cached total height of view
	Win::Rect			_layoutRect;
};

#endif
