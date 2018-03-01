#if !defined viewman_h
#define viewman_h
// ----------------------------------
// (c) Reliable Software, 2000 - 2006
// ----------------------------------

#include "SelectionMan.h"
#include "View.h"
#include "TableView.h"
#include "Tab.h"
#include "PreferencesStorage.h"
#include "ViewType.h"

#include <Ctrl/Menu.h>
#include <Ctrl/Accelerator.h>
#include <Ctrl/ToolBar.h>
#include <Ctrl/InfoDisp.h>
#include <Ctrl/StatusBar.h>
#include <Win/Win.h>
#include <stack>

namespace Win { class MessagePrepro; }

class TableProvider;
class Restriction;

class ViewMan : public SelectionMan
{
public:
	ViewMan (Win::Dow::Handle win, 
			 Cmd::Vector & cmdVector, 
			 TableProvider & tableProvider, 
			 Cmd::Executor & executor);

	void OnStartup ();
	void OnDestroy ();

	Tab::Handle &	GetTabView () { return _tabs; }
	Tool::Bar &		GetToolBarView () { return _toolbar; }
	Win::ListView & GetItemView () { return _itemView; }
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();

	void Size (int dx, int dy);
    void RefreshAll ();
	void Refresh (ViewType view);
	void Sort (unsigned int sortCol);
	void SetFocus ();

	void AttachMenu (Win::Dow::Handle hwnd) { _menu.AttachToWindow (hwnd); }
	void AttachAccelerator (Win::Dow::Handle hwnd, Win::MessagePrepro * msgPrepro);

 	void DisplayHelp (char const * helpMsg) { _statusBar.SetText (helpMsg); }
	void ResetStatusText () { _statusBar.SetText ("Ready"); }
    void RefreshPopup (Menu::Handle menu, int barItemNo);
    void DisplayTaskbarIconMenu ();
	bool DisplayContextMenu (Win::Dow::Handle hwndCmd, Win::Dow::Handle hwndClicked, int x, int y);
	
	void ShowWindow ();
	void HideWindow ();
	void SelectTab (ViewType newView);
	void SwitchToNextTab (bool isForward);
	void TabChanged ();
	void GoUp ();
	void GoDown (Restriction const * restrict);
    bool IsIn (ViewType view) const;

    void FillToolTip (Tool::TipForCtrl * tip) const;
    void FillToolTip (Tool::TipForWindow * tip) const;
	void UpdateToolBarState ();

    // Selection management
    bool HasSelection () const;
    void GetSelectedRows (std::vector<int> & rows) const;
	RecordSet const * GetRecordSet (char const * tableName) const;
	Restriction const & GetRestriction () const;

	void NonClientMetricsChanged ();

private:
	void SwitchTab (ViewType newTab);
	void UpdateDisplay (Restriction const * restrict = 0);
	void RefreshCurrentView ();
	void RefreshToolbar ();
	void RefreshTabs ();
	void ResizeToolbarCaption ();
	ViewType GetCurViewType () const;

	void UseSystemSettings ();

private:
	Win::Dow::Handle	_appWin;
	Cmd::Vector &		_cmdVector;

	Menu::DropDown		_menu;
	std::unique_ptr<Accel::Handler>	_kbdAccel;
	MainWinTabs			_tabs;
	Tool::Bar			_toolbar;
	Tool::ButtonCtrl	_buttonCtrl;
	Win::InfoDisplay	_caption;
	Win::StatusBar		_statusBar;
	ItemView			_itemView;

	Preferences::TopWinPlacement	_placement;

	TableView		_quarantine;
	TableView		_alertLog;
	TableView		_publicInbox;
	TableView		_members;
	TableView		_projectMembers;
	TableView		_remoteHubs;

	static unsigned int const TabCount = 5;

	std::stack<ViewType>	_publicInboxTabViewStack;
	std::stack<ViewType> 	_quarantineTabViewStack;
	std::stack<ViewType> 	_alertLogTabViewStack;
	std::stack<ViewType>	_memberTabViewStack;
	std::stack<ViewType>	_hubTabViewStack;
	std::stack<ViewType> *	_curTabViewStack;
	TableView *				_curView;
	// helper mappings
	TableView *				_type2View  [ViewCount];
	std::stack<ViewType> * 	_type2Stack [TabCount];
	Menu::Context			_taskbarIconMenu;
};

#endif
