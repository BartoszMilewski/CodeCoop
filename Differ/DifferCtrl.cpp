//------------------------------------
//  (c) Reliable Software, 1996 - 2008
//------------------------------------
#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "DifferCtrl.h"
#include "resource.h"
#include "DiffMain.h"
#include "AccelTab.h"
#include "CmdVector.h"
#include "OutputSink.h"
#include "EditParams.h"
#include "MenuTable.h"
#include "Registry.h"
#include "BrushBox.h"
#include "CommonUserMsg.h"
#include "UiDifferIds.h"
#include "CheckOutDlg.h"
#include "Global.h"

#include <Dbg/Out.h>
#include <Dbg/Log.h>
#include <Ex/WinEx.h>
#include <Win/WinMaker.h>
#include <Win/MsgLoop.h>
#include <Win/Metrics.h>
#include <Ctrl/Splitter.h>
#include <Ctrl/Messages.h>
#include <Ctrl/FontSelector.h>
#include <Graph/Canvas.h>
#include <Graph/Brush.h>
#include <Graph/CanvTools.h>

char const * DifferCtrl::_onePaneTopWinPrefrencesPath =
	"Software\\Reliable Software\\Code Co-op\\Preferences\\Differ\\Main Window";
char const * DifferCtrl::_twoPanesTopwinPrefrencesPath =
	"Software\\Reliable Software\\Code Co-op\\Preferences\\Differ\\Split Window";

DifferCtrl::DifferCtrl (Win::Instance hInstance, Win::MessagePrepro & msgPrepro, std::unique_ptr<XML::Tree> xmlArgs)
	: _splitterUser (hInstance),
	  _msgPrepro (msgPrepro),
	  _xmlArgs (std::move(xmlArgs)),
	  _fileNames (_xmlArgs->GetRoot ()),
	  _hwndFocus (0),
	  _docLine (1),
	  _docCol (1),
	  _splitRatio (50),
	  _twoPaneSplitRatio (50),
	  _hiddenSplitter (true),
	  _msgStartup (UM_STARTUP),
	  _upAndRunning (false)
{}

DifferCtrl::~DifferCtrl ()
{}

Notify::Handler * DifferCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_instrumentBar.get () != 0 && _instrumentBar->IsHandlerFor (idFrom))
		return _instrumentBar.get ();
	else if (_tabs.get () != 0 && _tabs->IsHandlerFor (idFrom))
		return _tabs.get ();
	else
		return _toolTipHandler.get ();
}

Control::Handler * DifferCtrl::GetControlHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	return _instrumentBar->GetControlHandler (winFrom, idFrom);
}


bool DifferCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
#if defined DIAGNOSTIC
		// By default we log to debug monitor window 
		Dbg::TheLog.DbgMonAttach ("Differ");
#endif
		// Initializes _viewWin by sending message
		_editorPool.Init (_h);
		_diffWin = _editorPool.GetDiffWindow ();
		// Create command execution objects
		_commander.reset (new Commander (_h,
										 _viewWin,
										 _hwndFocus,
										 _editorPool,
										 _fileNames,
										 _msgPrepro));
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		Accel::Maker findAccelMaker (Accel::KeysDialogFind, *_cmdVector);
		_kbdDlgFindAccel = findAccelMaker.Create ();
		_findPrompter.reset (new FindPrompter (_msgPrepro, _h, _kbdDlgFindAccel));
		_commander->AddFindPrompter (_findPrompter.get ());
		// Create user interface objects
		_tabs.reset (new FileTabController (_h, *this));
		_commander->AddTabView (&_tabs->GetView ());
		_instrumentBar.reset (new InstrumentBar (_h,
												 *_cmdVector,
												 *this));
		_toolTipHandler.reset (new Tool::DynamicRebar::TipHandler (DIFFER_TOOL_TIP_HANDLER_ID, _h));
		_toolTipHandler->AddRebar (_instrumentBar.get ());
		Accel::Maker accelMaker (Accel::Keys, *_cmdVector);
		_kbdAccel.reset (new Accel::Handler (_h, accelMaker.Create ()));
		_menu.reset (new Menu::DropDown (Menu::barItems, *_cmdVector));

		// Set keyboard accelerators
		_msgPrepro.SetKbdAccelerator (_kbdAccel.get ());

		// Create status bar
		_statusBarHandler.Attach (_h, ID_STATUSBAR);
		Win::StatusBarMaker statusMaker (_h, ID_STATUSBAR);
		statusMaker.Style () << Win::Style::Ex::ClientEdge;
		_statusBar.Reset (statusMaker.Create ());
		_statusBar.AddPart (28);
		_statusBar.AddPart (10);
		// legend
		for (int i = 0; i < 10; ++i)
			_statusBar.AddPart (6);

		_splitter = _splitterUser.MakeSplitter (_h, ID_SPLITTER);
		// Attach menu to the main window
		_menu->AttachToWindow (_h);
		if (_commander->IsDualPaneDisplay ())
		{
			// if two panes
			_topWinPlacementPrefs.reset (new Preferences::TopWinPlacement
				(_h, _twoPanesTopwinPrefrencesPath));
			Registry::UserDifferPrefs prefs;
			prefs.GetSplitRatio (_splitRatio);
			if (_splitRatio < 0 || _splitRatio > 100)
				_splitRatio = 50;
			_twoPaneSplitRatio = _splitRatio;
		}
		else
		{
			// if is only one pane
			_splitter.Hide ();
			_topWinPlacementPrefs.reset (new Preferences::TopWinPlacement
				(_h, _onePaneTopWinPrefrencesPath));
		}
		Assert (_topWinPlacementPrefs.get () != 0);
		_topWinPlacementPrefs->PlaceWindow (_cmdShow, _manyDiffers);
		_h.PostMsg (_msgStartup);
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Window Creation -- Unknown Error", Out::Error); 
	}
	TheOutput.SetParent (_h);
	return true;
}

void DifferCtrl::OnStartup ()
{
	_upAndRunning = Initialize ();
}

bool DifferCtrl::OnDestroy () throw ()
{
	try
	{
		if (_upAndRunning)
		{
			// Store UI preferences only when properly initialized
			UpdatePreferences ();
			_topWinPlacementPrefs.reset (0);
		}
	}
	catch (...)
	{
		Win::ClearError ();
	}
	Win::Quit ();
	return true;
}

bool DifferCtrl::OnClose () throw ()
{
	if (_commander->CheckForEditChanges ())
	{
		_h.Destroy ();
	}
	return true;
}

bool DifferCtrl::OnFocus (Win::Dow::Handle winPrev) throw ()
{
    _hwndFocus.SetFocus ();
	VerifyProjectFileInfo ();
	return true;
}

bool DifferCtrl::OnSize (int width, int height, int flag) throw ()
{
	_cx = width;
	_cy = height;
    int y = 0;
	// Toolbar
	int dy = _instrumentBar->Height ();
	Win::Rect instrumentBarRect (0, 0, width, dy);
	_instrumentBar->Size (instrumentBarRect);
    y += dy;

	// Tabs
	Win::Rect tabRect (0, 0, width, y); // full client area rectangle
	_tabs->GetView ().GetDisplayArea (tabRect); // rectangle, which we can use
	int tabsHeight = y - tabRect.Height (); 
	_tabs->GetView ().ReSize  (0, y, width, tabsHeight);
	y += tabsHeight;

	dy = height - y - _statusBar.Height () - 3;
	// Splitter, edit and diff windows
	int splitterWidth = _hiddenSplitter ? 0 : splitWidth;
	Assert (0 <= _splitRatio && _splitRatio <= 100);
	int xSplit = (_cx * _splitRatio) / 100;
	if (xSplit == _cx)
		xSplit -= splitterWidth;
	_splitter.MoveDelayPaint (xSplit, y, splitterWidth, dy);
	_editRect = Win::Rect (0, y, xSplit, y + dy);
	_viewWin.Move (_editRect);
	_diffWin.Move (xSplit + splitterWidth, y, width - xSplit - splitterWidth, dy);
	// Status bar
    y += dy + 3;
    dy = _statusBar.Height ();
    _statusBar.ReSize (0, y, width, dy);
	_splitter.ForceRepaint ();
	return true;
}

bool DifferCtrl::OnActivate (bool isClickActivate, bool isMinimized, Win::Dow::Handle prevWnd) throw ()
{
	_hwndFocus.SetFocus ();
	VerifyProjectFileInfo ();
	_instrumentBar->RefreshButtons ();
	return true;
}

bool DifferCtrl::OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ()
{
	activate.SetActivate ();
	return true;
	// Child should do this?
	if (hitTest.IsClient ())
		activate.SetActivateAndEat (); // don't pass mouse message
	else
		activate.SetActivate ();
	return true;
}

bool DifferCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (isAccel)
	{
		// Keyboard accelerators to invisible or disabled menu items are not executed
		if (_cmdVector->Test (cmdId) != Cmd::Enabled)
			return true;
	}
	MenuCommand (cmdId);
	return true;
}

bool DifferCtrl::IsEnabled (std::string const & cmdName) const throw ()
{
	return (_cmdVector->Test (cmdName.c_str ()) == Cmd::Enabled);
}

void DifferCtrl::ExecuteCommand (std::string const & cmdName) throw ()
{
	if (IsEnabled (cmdName))
	{
		int cmdId = _cmdVector->Cmd2Id (cmdName.c_str ());
		Assert (cmdId != -1);
		MenuCommand (cmdId);
	}
}

void DifferCtrl::ExecuteCommand (int cmdId) throw ()
{
	Assert (cmdId != -1);
	MenuCommand (cmdId);
}

bool DifferCtrl::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
		if (IsReadOnly (_viewWin))
			_statusBar.SetText ("Read-only");
		else
			_statusBar.SetText ("Ready");
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
		_statusBar.SetText (cmdHelp);
	}
	else
	{
		return false;
	}
	return true;
}

bool DifferCtrl::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	_menu->RefreshPopup (menu, pos);
	return true;
}

bool DifferCtrl::OnWheelMouse (int zDelta) throw ()
{
    if (_wheelMouse.IsSupported ())
    {
        int code = zDelta < 0 ? SB_LINEDOWN : SB_LINEUP;
        int scrollLines = _wheelMouse.GetScrollIncrement ();
        for (int i = 0; i < scrollLines; i++)
            _hwndFocus.SendMsg (WM_VSCROLL, MAKEWPARAM (code, 0));
		return true;
    }
	return false;
}

bool DifferCtrl::OnUserMessage (Win::UserMessage & msg) throw ()
{
    switch (msg.GetMsg ())
    {
	case UM_GIVEFOCUS:
		{
			// Revisit: this is not portable
			Win::Dow::Handle win (reinterpret_cast<HWND> (msg.GetLParam ()));
			GiveFocus (win);
		}
		return true;
	case UM_VSPLITTERMOVE:
		MoveSplitter (msg.GetLParam ());
		return true;
	case UM_EXIT:
		Exit ();
		return true;
	case UM_SCROLL_NOTIFY:
		SynchScroll (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_DOCPOS_UPDATE:
		DocPosUpdate (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_HIDE_WINDOW:
		HideWindow (msg.GetWParam ());
		return true;
	case UM_REFRESH_UI:
		RefreshUI ();
		return true;
	case UM_CHECK_OUT:
		if (CheckOut ())
			msg.SetResult (1);
		else
			msg.SetResult (0);
		return true;
	case UM_FIND_NEXT:
	case UM_REPLACE:
	case UM_REPLACE_ALL:
		_hwndFocus.SendMsg (msg);
		return true;
	case UM_INIT_DLG_REPLACE:
		_commander->Search_Replace ();
		return true;
	case UM_UPDATE_PLACEMENT:
		 UpdatePreferences ();
		 return true;
    case UM_GOTO_PARAGRAPH:
		_hwndFocus.SendMsg (msg);
		return true;
	case UM_PREPARE_CHANGE_NUMBER_PANES:
		PrepareChangeNumberPanes ();
		return true;
	case UM_LAYOUT_CHANGE:
		OnSize (_cx, _cy, 0);
		return true;
	case UM_SET_CURRENT_EDIT_WIN:
		_viewWin = Win::Dow::Handle (reinterpret_cast<HWND> (msg.GetLParam ()));
		return true;
	}
	return false;
}

bool DifferCtrl::OnRegisteredMessage (Win::Message & msg) throw ()
{
	if (_wheelMouse.IsSupported () && _wheelMouse.IsVScroll (msg.GetMsg ()))
	{
		OnWheelMouse (msg.GetWParam ());
		return true;
	}
	else if (msg == _msgStartup)
	{
		dbg << "--> DifferCtrl::OnRegisteredMessage -- UM_STARTUP" << std::endl;
		OnStartup ();
		dbg << "<-- DifferCtrl::OnRegisteredMessage -- UM_STARTUP" << std::endl;
		return true;
	}
	return false;
}

bool DifferCtrl::IsReadOnly (Win::Dow::Handle win) const
{
	Win::UserMessage msg (UM_EDIT_IS_READONLY);
	win.SendMsg (msg);
	return msg.GetResult () != 0;
}

bool DifferCtrl::Initialize ()
{
	try
	{
		XML::Node const * root = _xmlArgs->GetRoot ();
		if (root)
		{
			XML::Node const * child = root->FindChildByAttrib ("file", "role", "current");
			if (child)
			{
				_commander->SetTitle(child->GetTransformAttribValue ("path"));
			}
			else
			{
				child = root->FindChildByAttrib ("file", "role", "after");
				if (child && child->FindAttribute ("display-path"))
					_h.SetText (child->GetTransformAttribValue ("display-path"));
			}
		}

		Registry::UserDifferPrefs prefs;
		int x1, y1, x2, y2;
		prefs.GetFindDlgPosition (x1, y1, x2, y2);
		_findPrompter->RememberDlgPositions (x1, y1, x2, y2);
		
		NonClientMetrics metrics;
		if (metrics.IsValid ())
		{
			_instrumentBar->SetFont (metrics.GetStatusFont ());
			_tabs->GetView ().SetFont (metrics.GetMenuFont ());
		}

		// Initialize commander -- can be done after all user prefs are read
		_commander->InitViewing ();
		_instrumentBar->RefreshButtons ();
		_statusBar.SetText ("Local:", 2, false);
		_statusBar.SetText ("Sync:", 7, false);
		for (int i = 0; i < 4; ++i)
		{
			_statusBar.SetOwnerDraw (&_statusBarHandler, 3+i);
			_statusBar.SetOwnerDraw (&_statusBarHandler, 8+i);
		}
		_hwndFocus = _viewWin;
		_hwndFocus.SetFocus ();
		_findPrompter->SetCanEdit (_commander->CanEdit ());
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		return false;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error); 
		return false;
	}
	return true;
}

bool DifferCtrl::StatusBarHandler::Draw (Win::Canvas canvas, Win::Rect const & rect, int itemId) throw ()
{
	Win::TransparentBkgr bkgr (canvas);
	BrushBox brushBox;
	std::string text;
	EditStyle::Action act;
	switch (itemId)
	{
	case 3:
		text = "New";
		act = EditStyle::actInsert;
		break;
	case 4:
		text = "Paste";
		act = EditStyle::actPaste;
		break;
	case 5:
		text = "Cut";
		act = EditStyle::actCut;
		break;
	case 6:
		text = "Delete";
		act = EditStyle::actDelete;
		break;
	case 8:
		text = "New";
		act = EditStyle::actInsert;
		break;
	case 9:
		text = "Paste";
		act = EditStyle::actPaste;
		break;
	case 10:
		text = "Cut";
		act = EditStyle::actCut;
		break;
	case 11:
		text = "Delete";
		act = EditStyle::actDelete;
		break;
    default:
        throw Win::Exception("Unknown edit style.");
	}
	EditStyle::Source src = (itemId < 8)? EditStyle::chngUser: EditStyle::chngSynch;
	EditStyle style (src, act);

	Font::ColorHolder textColor (canvas, brushBox.GetTextColor (style));
	Brush::AutoHandle brush (brushBox.GetBkgColor (style));
	// Revisit: untangle canvas and brush dependencies
	canvas.FillRect (rect, brush.ToNative ());
	canvas.Text (rect.left+2, rect.top+1, text.c_str (), text.size ());
#if 0
	if (style.IsRemoved ())
	{
		int y = (rect.bottom + rect.top + 2) / 2;
		Pen::Black pen (canvas);
		canvas.Line (rect.left+2, y, rect.right-2, y);
	}
#endif
	return true;
}

void DifferCtrl::MenuCommand (int cmdId)
{
	try
	{
		Cursor::Holder working (_hourglass);

		_cmdVector->Execute (cmdId);
		if (IsReadOnly (_viewWin))
			_statusBar.SetText ("Read-only");
		else
			_statusBar.SetText ("Ready");
		_instrumentBar->RefreshButtons ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal Error: Command execution failure", Out::Error);
	}
}

void DifferCtrl::GiveFocus (Win::Dow::Handle hwndChild)
{
	Assert (hwndChild == _viewWin || hwndChild == _diffWin);
	_hwndFocus = hwndChild;
    _hwndFocus.SetFocus ();
	_findPrompter->SetCanEdit (_commander->CanEdit ());
	_instrumentBar->RefreshButtons ();
}

void DifferCtrl::MoveSplitter (int x)
{
	_splitRatio = x * 100 / _cx;
	if (_splitRatio < 0)
		_splitRatio = 0;
	else if (_splitRatio > 100)
		_splitRatio = 100;
	_twoPaneSplitRatio = _splitRatio;
	OnSize (_cx, _cy, 0);
}

void DifferCtrl::HideWindow (int wndCode)
{
	bool hideLeft = (wndCode == 1);
	bool hideRight = (wndCode == 2);
	if (hideLeft)
	{
		// Hide edit window
		_viewWin.Hide ();
		_diffWin.Show ();
		if (_splitRatio != 0 && _splitRatio != 100)
			_twoPaneSplitRatio = _splitRatio;
		_splitRatio = 0;
		_hwndFocus = _diffWin;
		_statusBar.SetText ("Read-only");
	}
	else if (hideRight)
	{
		// Hide diff window
		_diffWin.Hide ();
		_viewWin.Show ();
		if (_splitRatio != 0 && _splitRatio != 100)
			_twoPaneSplitRatio = _splitRatio;
		_splitRatio = 100;
		_hwndFocus = _viewWin;
		if (IsReadOnly (_viewWin))
			_statusBar.SetText ("Read-only");
		else
			_statusBar.SetText ("Ready");
	}
	else
	{
		// Don't hide any
		_viewWin.Show ();
		_diffWin.Show ();
		_splitRatio = _twoPaneSplitRatio;
		if (IsReadOnly (_viewWin))
			_statusBar.SetText ("Read-only");
		else
			_statusBar.SetText ("Ready");
	}
	// Hide splitter if necessary
	_hiddenSplitter = hideLeft || hideRight;
	if (_hiddenSplitter)
		_splitter.Hide ();
	else
		_splitter.Show ();

	_topWinPlacementPrefs->PlaceWindow ();
	_findPrompter->SetPaneMode (_hiddenSplitter);
	_fileNames.GetSccStatus ();
	// Refresh toolbar buttons and info
	_instrumentBar->RefreshFileDetails (GetSummary (), GetFullInfo ());		
	_instrumentBar->RefreshButtons ();
	OnSize (_cx, _cy, 0);
	_hwndFocus.SetFocus ();
	_viewWin.Invalidate ();
	_diffWin.Invalidate ();

	RefreshLineColumnInfo ();
	_findPrompter->SetCanEdit (_commander->CanEdit ());
}

void DifferCtrl::RefreshUI ()
{
	_instrumentBar->RefreshButtons ();
}

void DifferCtrl::SynchScroll (int offset ,int targetPara)
{
	Win::UserMessage msg (UM_SYNCH_SCROLL, offset, targetPara);
	_editorPool.SendEditorMsgExcept (msg, _hwndFocus);
}

void DifferCtrl::DocPosUpdate (int bar, int pos)
{
	// Display only position in the edit (left) window
	if (bar == SB_VERT)
		_docLine = pos;
	else if (bar == SB_HORZ)
		_docCol = pos;

	RefreshLineColumnInfo ();
}

bool DifferCtrl::CheckOut ()
{
	if (!_fileNames.HasFiles ())
		return true; // no files means new nameless buffer

	if (!_fileNames.IsControlledFile ())
	{
		return _fileNames.HasPath (FileCurrent) 
			&& !File::IsReadOnly (_fileNames.GetPath (FileCurrent));
	}

	Assert (_fileNames.ProjectIsPresent ());
	Assert (_fileNames.IsReadOnlyFile () && _fileNames.IsControlledFile ());

	bool doCheckOut = false;
	Registry::UserDifferPrefs prefs;
	if (prefs.IsNoCheckoutPrompt ())
		doCheckOut = true;
	else
	{
		bool dontAsk = false;
		CheckOutCtrl ctrl (dontAsk);
		Dialog::Modal dialog (_h, ctrl);
		if (dialog.IsOK ())
		{
			doCheckOut = true;
			if (dontAsk)
				prefs.SetNoCheckoutPrompt (true);
		}
	}

	if (doCheckOut)
	{
		Cursor::Holder working (_hourglass);
		_commander->File_CheckOut ();
		if (!_fileNames.IsReadOnlyFile ())
		{
			//_differInfo.SetLocalEdit (true);
			_statusBar.SetText ("Ready");
		}
		_instrumentBar->RefreshFileDetails (GetSummary (), GetFullInfo ());
	}

	return !_fileNames.IsReadOnlyFile ();
}

// Called on every focus change
void DifferCtrl::VerifyProjectFileInfo ()
{
	if (!_fileNames.ProjectIsPresent ())
		return;

	_findPrompter->SetCanEdit (_commander->CanEdit ());

	// Check the information about file displayed in the editable window
	if (_fileNames.GetProjectFileReadOnlyState ())
	{
		// Read only change detected
		bool isReadOnly = _fileNames.IsReadOnlyFile ();
		_editorPool.SetEditableReadOnly (isReadOnly);

		if (isReadOnly)
			_fileNames.GetSccStatus (true);

		if (IsReadOnly (_viewWin))
			_statusBar.SetText ("Read-only");
		else
			_statusBar.SetText ("Ready");
	}
	if (_fileNames.GetProjectFileTimeStamp ())
	{
		// File time stamp change detected
		std::string info;
		info += _fileNames.Project ();
		info += "\n\nThe file has been modified outside of the Differ. Do you want to reload it ?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
												   Out::PromptStyle (Out::YesNo,
																	 Out::Yes,
																	 Out::Question),
												   _h);
		if (userChoice == Out::Yes)
		{
			try
			{
				_commander->InitViewing (true);// true refresh
			}
			catch (Win::Exception & e)
			{
				TheOutput.Display (e);
			}
		}
	}
}

void DifferCtrl::UpdatePreferences ()
{
	Registry::UserDifferPrefs regPrefs;
	regPrefs.SaveLineBreaking (_editorPool.IsLineBreakingOn ());
	if (_hiddenSplitter)
	{
		_findPrompter->SetPaneMode (true);
		int xLeft, yTop;
		if (_findPrompter->GetDlgPosition (xLeft, yTop))// dialog was open 
			regPrefs.SaveFindDlgPosition (xLeft, yTop, 1);
	}
	else 
	{
		_findPrompter->SetPaneMode (false);
		// Save _splitRatio only if it's in range 0 to 100
		if (0 <= _splitRatio && _splitRatio <= 100)
			regPrefs.SetSplitRatio (_splitRatio);		
		int xLeft, yTop;
		if (_findPrompter->GetDlgPosition (xLeft, yTop))// dialog was open 
			regPrefs.SaveFindDlgPosition (xLeft, yTop, 2);		
    }
}

std::string DifferCtrl::GetSummary () const
{
	if (!_fileNames.HasFiles ())
		return "No files selected";

	if (_fileNames.IsControlledFile () || _fileNames.IsCheckedoutFile ())
	{
		if (_fileNames.IsControlledFile ())
			return "Checked in";
		else if (_fileNames.IsCheckedoutFile ())
			return "Checked out";
	}

	return "Not controlled";
}

std::string DifferCtrl::GetFullInfo () const
{
	std::string fullInfo;
#if 0
	if (_fileNames.IsComparing ())
	{
		fullInfo += "Comparing Files\n";
		std::string left ("Left pane:  \"");
		if (!_differInfo.GetCurrentName ().empty ())
			left += _differInfo.GetCurrentName ();
		else
			left += _fileNames.Project ();
		left += "\"\n";
		fullInfo += left;

		std::string right ("Comparing with (right pane): \"");
		right += _fileNames.FileForComparing ();
		right += "\"\n";
		fullInfo += right;
		if (_hiddenSplitter)
			fullInfo += "Files Are Identical";
		else
			fullInfo += "Files Are Different";
	}
	else
	{
		if (_differInfo.IsHistorical ())
			fullInfo += "Newer File:\n";
		else if (_fileNames.ProjectIsPresent ())
			fullInfo += "Current File:\n";

		if (_fileNames.ProjectIsPresent ())
		{
			std::string name ("Name:  \"");
			if (!_differInfo.GetCurrentName ().empty ())
				name += _differInfo.GetCurrentName ();
			else
				name += std::string (_fileNames.Project ());
			name += "\"";
			fullInfo += name;

			std::string time;
			std::string size;
			_fileNames.GetProjectFileInfo (time, size);
			std::string timeLine ("\nLast modified: ");
			timeLine += time;
			fullInfo += timeLine;
			std::string sizeLine ("\nSize: ");
			sizeLine += size;
			fullInfo += sizeLine;

			if (!_differInfo.GetCurrentType ().empty ())
			{
				std::string type ("\nType: \"");
				type += _differInfo.GetCurrentType ();
				type += "\"";
				fullInfo += type;
			}

			std::string attr ("\nAttribute: ");
			attr += _fileNames.GetProjectFileAttribute ();
			fullInfo += attr;		
		}

		_differInfo.DumpLocalChanges (fullInfo);
		_differInfo.DumpSynchChanges (fullInfo);
	}
#endif
	return fullInfo;
}

void DifferCtrl::PrepareChangeNumberPanes ()
{
	if (_hiddenSplitter)
	{
		// if is only one pane
		// preparing : one pane -> two panes
		_topWinPlacementPrefs.reset (new Preferences::TopWinPlacement
			(_h, _twoPanesTopwinPrefrencesPath));
	}
	else
	{
		// if two panes
		// preparing : two panes -> one pane
		_topWinPlacementPrefs.reset (new Preferences::TopWinPlacement
			(_h, _onePaneTopWinPrefrencesPath));
	}
	_topWinPlacementPrefs->Update ();
}

void DifferCtrl::ChangeFileView (FileSelection page)
{
	// TheOutput.Display (_fileNames.GetPath (page));
	_viewWin.Hide ();
	_editorPool.SwitchWindow (page);
	_viewWin.Move (_editRect);
	_viewWin.Show ();
	GiveFocus (_viewWin);
	RefreshStatus ();
}

void DifferCtrl::RefreshLineColumnInfo ()
{
	std::string info ("Ln ");
	info += ToString (_docLine);
	info += ", Col ";
	info += ToString (_docCol);
	_statusBar.SetText (info.c_str (), 1);
}

void DifferCtrl::RefreshStatus ()
{
	Win::UserMessage msgCurrPos (UM_GETDOCPOSITION);
	msgCurrPos.SetWParam (SB_VERT);
    _viewWin.SendMsg (msgCurrPos);
    _docLine = msgCurrPos.GetResult () + 1;
	msgCurrPos.SetWParam (SB_HORZ);
    _viewWin.SendMsg (msgCurrPos);
    _docCol = msgCurrPos.GetResult () + 1;

	if (IsReadOnly (_viewWin))
		_statusBar.SetText ("Read-only");
	else
		_statusBar.SetText ("Ready");
	RefreshLineColumnInfo ();
}
