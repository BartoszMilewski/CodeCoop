//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include <WinLibBase.h>
#include "ToolBar.h"
#include <Win/Message.h>

using namespace Tool;
using namespace Notify;

// A window that is positioned over its parent window and uses its tool tip control
class OveralyWindow : public TOOLINFO
{
public:
	OveralyWindow (Win::Dow::Handle hwndTool, Win::Dow::Handle toolTipHandlerWin);
};

OveralyWindow::OveralyWindow (Win::Dow::Handle hwndTool, Win::Dow::Handle toolTipHandlerWin)
{
	memset (this, 0, sizeof (TOOLINFO));
	cbSize = sizeof (TOOLINFO);
	uFlags = TTF_IDISHWND | TTF_SUBCLASS;	// Tool id is a window handle. 
	// Subclass tool window to intercept mouse moves
	uId = reinterpret_cast<UINT>(hwndTool.ToNative ());	// Window added to the tool bar
	hwnd = toolTipHandlerWin.ToNative ();	// Window that receives tool tip notifications
	lpszText = LPSTR_TEXTCALLBACK;			// Send TTN_NEEDTEXT message to the handler window
}

bool Handle::GetMaxSize (unsigned & width, unsigned & height) const
{
	SIZE sizeInfo;
	Win::Message msg (TB_GETMAXSIZE, 0, reinterpret_cast<LPARAM>(&sizeInfo));
	SendMsg (msg);
	if (msg.GetResult () != 0)
	{
		width = sizeInfo.cx;
		height = sizeInfo.cy;
	}
	return msg.GetResult () != 0;
}

void Handle::ClearButtons ()
{
	while (Delete (0))
		continue;
}

void Handle::AddWindow (Win::Dow::Handle overlayWin, Win::Dow::Handle toolTipHandlerWin)
{
	OveralyWindow  toolWnd (overlayWin, toolTipHandlerWin);
    HWND hwndTT = reinterpret_cast<HWND> (SendMsg (TB_GETTOOLTIPS, 0, 0));
	if (hwndTT == 0)
		throw Win::Exception ("Internal error: Cannot add window tool to the tool bar.");
    ::SendMessage (hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolWnd));
}

void Handle::GetButtonRect (int buttonIdx, Win::Rect & rect) const
{
	SendMsg (TB_GETITEMRECT, (WPARAM) buttonIdx, (LPARAM) (&rect));
}

int Handle::CmdIdToButtonIndex (int cmdId)
{
	return SendMsg (TB_COMMANDTOINDEX, (WPARAM) cmdId);
}

unsigned Handle::GetToolTipDelay () const
{
    HWND hwndTT = reinterpret_cast<HWND> (SendMsg (TB_GETTOOLTIPS, 0, 0));
	if (hwndTT == 0)
        throw Win::Exception("Internal error: Cannot get the tool bar tool tip delay.");
    return ::SendMessage (hwndTT, TTM_GETDELAYTIME, TTDT_AUTOPOP, 0);
}

void Handle::SetToolTipDelay (unsigned milliSeconds) const
{
    HWND hwndTT = reinterpret_cast<HWND> (SendMsg (TB_GETTOOLTIPS, 0, 0));
	if (hwndTT == 0)
        throw Win::Exception("Internal error: Cannot set the tool bar tool tip delay.");
    ::SendMessage (hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, milliSeconds);
}

void Handle::InsertSeparator (int idx, int width)
{
	BarSeparator separator (width);
	SendMsg (TB_INSERTBUTTON, idx, reinterpret_cast<LPARAM>(&separator));
}

//----------
// Tool::Bar
//----------
Bar::Bar (Win::Dow::Handle winParent,
		  int toolbarId,
		  int bitmapId,
		  int buttonWidth,
		  Cmd::Vector const & cmdVector,
		  Tool::Item const * buttonItems,
		  Win::Style const & barStyle)
	: _buttonImages (winParent.GetInstance (), bitmapId, buttonWidth),
	  _cmdVector (cmdVector),
	  _buttonItems (buttonItems)
{
	Tool::Maker maker (winParent, toolbarId);
	maker.Style () << barStyle;
	Reset (maker.Create ());
	int width, height;
	_buttonImages.GetImageSize (width, height);
	SetButtonSize (width, height);
	SetImageList (_buttonImages);

	// Map button id to command id and cmd id to button item index
	for (unsigned i = 0; _buttonItems [i].buttonId != Item::idEnd; ++i)
	{
		int buttonId = _buttonItems [i].buttonId;
		if (buttonId != Item::idSeparator)
		{
			int cmdId = _cmdVector.Cmd2Id (_buttonItems [i].cmdName);
			Assert (cmdId != -1);
			_buttonId2CmdId [buttonId] = cmdId;
			_cmdId2ButtonIdx [cmdId] = i;
		}
	}
}

void Bar::SetAllButtons ()
{
	ClearButtons ();
	_curCmdIds.clear ();
	std::vector<Tool::Button> buttons;
	for (unsigned i = 0; _buttonItems [i].buttonId != Item::idEnd; ++i)
	{
		int buttonId = _buttonItems [i].buttonId;
		if (buttonId == Item::idSeparator)
		{
			buttons.push_back (Tool::BarSeparator ());
		}
		else
		{
			Assert (_buttonId2CmdId.find (buttonId) != _buttonId2CmdId.end ());
			int cmdId = _buttonId2CmdId [buttonId];
			_curCmdIds.push_back (cmdId);
			buttons.push_back (Tool::BarButton (buttonId, cmdId));
		}
	}
	AddButtons (buttons);
}

void Bar::SetLayout (int const * layout)
{
	ClearButtons ();
	_curCmdIds.clear ();
	std::vector<Tool::Button> buttons;
	for (int i = 0; layout [i] != Item::idEnd; ++i)
	{
		int buttonId = layout [i];
		if (buttonId == Item::idSeparator)
		{
			buttons.push_back (Tool::BarSeparator ());
		}
		else
		{
			Assert (_buttonId2CmdId.find (buttonId) != _buttonId2CmdId.end ());
			int cmdId = _buttonId2CmdId [buttonId];
			buttons.push_back (Tool::BarButton (buttonId, cmdId));
			_curCmdIds.push_back (cmdId);
		}
	}
	AddButtons (buttons);
}

void Bar::Refresh ()
{
	for (std::vector<int>::const_iterator iter = _curCmdIds.begin (); iter != _curCmdIds.end (); ++iter)
	{
		int buttonCmdId = *iter;
		Cmd::Status cmdState = _cmdVector.Test (buttonCmdId);
		if (cmdState == Cmd::Enabled)
		{
			// Could be hidden and/or checked and now it is only enabled
			ShowButton (buttonCmdId);
			ReleaseButton (buttonCmdId);
			EnableButton (buttonCmdId);
		}
		else if (cmdState == Cmd::Checked)
		{
			// Could be hidden and/or disabled and now is checked
			ShowButton (buttonCmdId);
			EnableButton (buttonCmdId);
			PressButton (buttonCmdId);
		}
		else if (cmdState == Cmd::Disabled)
		{
			// Could be hidden
			ShowButton (buttonCmdId);
			DisableButton (buttonCmdId);
		}
		else
		{
			Assert (cmdState == Cmd::Invisible);
			HideButton (buttonCmdId);
		}
	}
}

unsigned Bar::GetButtonsEndX () const
{
	int lastIndex = ButtonCount () - 1;
	Win::Rect rect;
	GetButtonRect (lastIndex, rect);
	return rect.right;
}

unsigned Bar::GetButtonWidth (int idx) const
{
	Win::Rect rect;
	GetButtonRect (idx, rect);
	return rect.Width ();
}

void Bar::Disable () throw ()
{
	for (std::vector<int>::const_iterator iter = _curCmdIds.begin (); iter != _curCmdIds.end (); ++iter)
	{
		int buttonCmdId = *iter;
		DisableButton (buttonCmdId);
	}
}

void Bar::FillToolTip (Tool::TipForCtrl * tip) const
{
	int cmdId = tip->IdFrom ();
	std::map<int, int>::const_iterator iter = _cmdId2ButtonIdx.find (cmdId);
	Assert (iter != _cmdId2ButtonIdx.end ());
	int idx = iter->second;
	tip->SetText (_buttonItems [idx].tip);
}

bool Bar::IsCmdButton (int cmdId) const
{
	return std::find (_curCmdIds.begin (), _curCmdIds.end (), cmdId) != _curCmdIds.end ();
}

//
// Tool bar data
//

Button::Button ()
{
	memset (this, 0, sizeof (TBBUTTON));
	fsState = TBSTATE_ENABLED;
}

BarSeparator::BarSeparator (int width)
{
	fsStyle = TBSTYLE_SEP;
	iBitmap = width;
}

BarButton::BarButton (int buttonId, int cmdId)
{
	fsStyle = TBSTYLE_BUTTON;
    iBitmap = buttonId;
    idCommand = cmdId;
}

bool ToolTipHandler::OnNotify (NMHDR * hdr, long & result)
{
	// hdr->code
	// hdr->idFrom;
	// hdr->hwndFrom;
	switch (hdr->code)
	{
	case TTN_NEEDTEXT:
		{
			Tool::Tip * tip = reinterpret_cast<Tool::Tip *>(hdr); 
			if (tip->IsHwndFrom ())
			{
				return OnNeedText (reinterpret_cast<Tool::TipForWindow *>(tip));
			}
			else
			{
				Assert (tip->IsIdFrom ());
				return OnNeedText (reinterpret_cast<Tool::TipForCtrl *>(tip));
			}
		}
	}
	return false;
}

//------------------
// Button Controller
//------------------

bool Tool::ButtonCtrl::OnControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	// Notification from the tool bar button
	_executor.ExecuteCommand (id);
	return true;
}

void Tool::ButtonCtrl::AddButtonIds (Tool::Bar::ButtonIdIter first, Tool::Bar::ButtonIdIter last)
{
	std::copy (first, last, std::back_inserter (_myButtonIds));
}

bool Tool::ButtonCtrl::IsCmdButton (int id) const
{
	return std::find (_myButtonIds.begin (), _myButtonIds.end (), id) != _myButtonIds.end ();
}

