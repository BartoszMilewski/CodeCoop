#if !defined DIFFERCTRL_H
#define DIFFERCTRL_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2007
//------------------------------------
#include "FileNames.h"
#include "Commander.h"
#include "CmdVector.h"
#include "EditCtrl.h"
#include "InstrumentBar.h"
#include "PreferencesStorage.h"
#include "TabCtrl.h"
#include "EditorPool.h"

#include <Sys/WheelMouse.h>
#include <Ctrl/Accelerator.h>
#include <Ctrl/Menu.h>
#include <Ctrl/StatusBar.h>
#include <Ctrl/Scroll.h>
#include <Ctrl/InfoDisp.h>
#include <Ctrl/Splitter.h>
#include <Graph/Cursor.h>
#include <Win/Controller.h>
#include <Win/Win.h>

namespace Notify
{
	class Handler;
}

class Merger;

class DifferCtrl : public Win::Controller, public Cmd::Executor, public FileViewSelector
{
private:
	static unsigned const toolBarHeightAdjust = 4;

	class StatusBarHandler: public Win::StatusBar::DrawHandler
	{
	public:
		bool Draw (Win::Canvas canvas, Win::Rect const & rect, int itemId) throw ();
	};

public:
	DifferCtrl (Win::Instance hInstance, Win::MessagePrepro & msgPrepro, std::unique_ptr<XML::Tree> params);
    ~DifferCtrl ();
	void OnStartup ();
	// Win::Controller
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	Control::Handler * GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnClose () throw ();
	bool OnFocus (Win::Dow::Handle winPrev) throw ();
	bool OnSize (int width, int height, int flag) throw ();
	bool OnActivate (bool isClickActivate, bool isMinimized, Win::Dow::Handle prevWnd) throw ();
	bool OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ();

	bool OnCommand (int cmdId, bool isAccel) throw ();
	bool OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ();
	bool OnInitPopup (Menu::Handle menu, int pos) throw ();
	bool OnWheelMouse (int zDelta) throw ();
	bool OnUserMessage (Win::UserMessage & msg) throw ();
	bool OnRegisteredMessage (Win::Message & msg) throw ();

	// Cmd::Executor
	bool IsEnabled (std::string const & cmdName) const throw ();
	void ExecuteCommand (std::string const & cmdName) throw ();
	void ExecuteCommand (int cmdId) throw ();
	void DisableKeyboardAccelerators (Win::Accelerator *) throw () {}
	void EnableKeyboardAccelerators () throw () {}
	// FileViewSelector
	void ChangeFileView (FileSelection page);
	void SetPlacementParams (Win::ShowCmd cmdShow, bool manyDiffers)
	{
		_cmdShow = cmdShow;
		_manyDiffers = manyDiffers;
	}
private:
	bool Initialize ();
	bool IsReadOnly (Win::Dow::Handle win) const;
	void MenuCommand (int cmdId);
	void GiveFocus (Win::Dow::Handle hwndChild);
	void MoveSplitter (int x);
	void HideWindow (int wndCode);
	void RefreshUI ();
	void Exit () { _h.Destroy (); }
	void SynchScroll (int offset, int targetPara);
	void DocPosUpdate (int bar, int pos);
	void VerifyProjectFileInfo ();
	bool CheckOut ();
	void UpdatePreferences ();
	void PrepareChangeNumberPanes ();
	std::string GetSummary () const;
	std::string GetFullInfo () const;
	void RefreshLineColumnInfo ();
	void RefreshStatus ();

private:
	static unsigned int const splitWidth = 8; // Width of splitter
	static char const *		_onePaneTopWinPrefrencesPath;
	static char const *		_twoPanesTopwinPrefrencesPath;

private:
	Splitter::UseVertical	_splitterUser;

	std::unique_ptr<XML::Tree> _xmlArgs; // Must be before _fileNames!
	FileNames			_fileNames;
	EditorPool			_editorPool;

    // User Interface
	Win::Dow::Handle	_diffWin;			// Right window
    Win::Dow::Handle	_viewWin;		// Left window
    Win::Dow::Handle	_editableWin;		// Potentially editable window (CurrentFile)
	Win::Dow::Handle	_hwndFocus;
	Cursor::Hourglass	_hourglass;

	// Command execution
	std::unique_ptr<Commander>		_commander;
	std::unique_ptr<CmdVector>		_cmdVector;
	std::unique_ptr<Accel::Handler>	_kbdAccel;	      // main keyboard shortcuts
	Accel::AutoHandle				_kbdDlgFindAccel; // Find dialog keyboard shortcuts
	std::unique_ptr<Menu::DropDown>	_menu;
	std::unique_ptr<InstrumentBar>	_instrumentBar;
	std::unique_ptr<Tool::DynamicRebar::TipHandler>	_toolTipHandler;
	std::unique_ptr<FileTabController> _tabs;

	Win::Dow::Handle				_splitter;
	Win::StatusBar					_statusBar;
	StatusBarHandler				_statusBarHandler;
	Win::MessagePrepro &			_msgPrepro;	// Differ message pump
	std::unique_ptr<FindPrompter>		_findPrompter;

	int					_cx;
	int					_cy;
	Win::Rect			_editRect;
	int					_docLine;
	int					_docCol;
	int					_splitRatio;		// Current ratio, may also be 0 or 100 (in per cent)	
	int					_twoPaneSplitRatio;	// Last remembered two-pane ratio, cannot be 0 or 100
	bool				_hiddenSplitter;
	Win::ShowCmd		_cmdShow;
	bool				_manyDiffers;
	std::unique_ptr<Preferences::TopWinPlacement>	_topWinPlacementPrefs;	// Current top window preferences
    // IntelliMouse support
    IntelliMouse		_wheelMouse;
	// State
	bool				_upAndRunning;
	Win::RegisteredMessage	_msgStartup;
};

#endif
