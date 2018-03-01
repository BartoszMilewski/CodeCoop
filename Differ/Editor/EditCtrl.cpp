//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "EditCtrl.h"
#include "resource.h"
#include "EditParams.h"
#include "Lines.h"
#include "OutputSink.h"

#include <Ex/WinEx.h>
#include <File/MemFile.h>
#include <Win/WinMaker.h>
#include <Ctrl/MarginCtrl.h>

EditController::EditController (bool readOnlyWin)
   : _vScroll (0),
     _hScroll (0),
     _marginSize (::GetSystemMetrics (SM_CXVSCROLL)),
	 _hwndParent (0),
	 _marginWin (0),
	 _editPaneWin (0),
	 _isReadOnlyWin (readOnlyWin)
{}

EditController::~EditController ()
{}

bool EditController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		_hwndParent = create->GetParent ();
		_vScroll.Init (_h);
		_hScroll.Init (_h);
		// Create child windows
		Win::ChildMaker marginWinMaker (IDC_MARGIN, _h, ID_MARGIN);
		_marginWin = marginWinMaker.Create ();
		_marginWin.Show ();

		Win::ChildMaker editWinMaker (IDC_EDITPANE, _h, ID_EDIT_PANE);
		_editPaneWin = editWinMaker.Create (&_editPaneCtrl);
		_editPaneWin.Show ();
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Window Initialization -- Unknown Error", Out::Error); 
	}

	return true;
}

bool EditController::OnDestroy () throw ()
{
	Win::Quit ();
	return true;
}

bool EditController::OnFocus (Win::Dow::Handle winPrev) throw ()
{
    _editPaneWin.SetFocus ();
	return true;
}

bool EditController::OnSize (int width, int height, int flag) throw ()
{
	_cx = width;
	_cy = height;
    _marginWin.Move (0, 0, _marginSize, _cy);
    _editPaneWin.Move (_marginSize, 0, _cx - _marginSize, _cy);
	UpdScrollBars ();
	return true;
}

bool EditController::OnMouseActivate (Win::HitTest hitTest, Win::MouseActiveAction & activate) throw ()
{
	Win::UserMessage msg (UM_GIVEFOCUS);
	msg.SetLParam (_h);
	_hwndParent.SendMsg (msg);
	return false;	// Pass it to default window procedure
}

bool EditController::OnVScroll (int code, int pos, Win::Dow::Handle) throw ()
{
    int newPos = _vScroll.Command (code, pos);
	Win::UserMessage msg (UM_SETDOCPOSITION, SB_VERT, newPos);
    _editPaneWin.SendMsg (msg);
	return true;
}

bool EditController::OnHScroll (int code, int pos, Win::Dow::Handle) throw ()
{
    int newPos = _hScroll.Command (code, pos);
	Win::UserMessage msg (UM_SETDOCPOSITION, SB_HORZ, newPos);
    _editPaneWin.SendMsg (msg);
	return true;
}

bool EditController::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (!_hwndParent.IsNull ())
		_hwndParent.SendMsg (WM_COMMAND, static_cast<WPARAM>(cmdId));
	return true;
}

bool EditController::OnUserMessage (Win::UserMessage & msg) throw ()
{
    switch (msg.GetMsg ())
    {
	case UM_SYNCH_SCROLL:
		SynchScroll (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_UPDATESCROLLBARS:
		UpdScrollBars ();
		return true;
	case UM_SETSCROLLBAR:
		SetScrollBar (msg.GetWParam (), msg.GetLParam ());
		return true;
	case UM_CHECK_OUT:
		if (CheckOut ())
			msg.SetResult (1);
		else
			msg.SetResult (0);
		return true;
	case UM_SEARCH_NEXT:
	case UM_SEARCH_PREV:
	case UM_EDIT_COPY:
	case UM_EDIT_PASTE:
	case UM_EDIT_CUT:
	case UM_EDIT_DELETE:
	case UM_EDIT_NEEDS_SAVE:
	case UM_EDIT_IS_READONLY:
	case UM_SET_EDIT_READONLY:
	case UM_GET_BUF:
	case UM_INITDOC:
    case UM_STARTSELECTLINE:
    case UM_SELECTLINE:
    case UM_ENDSELECTLINE:
	case UM_CHANGE_FONT:
	case UM_GETDOCPOSITION:
	case UM_GETDOCDIMENSION:
	case UM_SELECT_ALL:
	case UM_GET_SELECTION:
	case UM_FIND_NEXT:
    case UM_TOGGLE_LINE_BREAKING:
	case UM_CLEAR_EDIT_STATE:
	case UM_REPLACE:
	case UM_REPLACE_ALL:
	case UM_GET_DOC_SIZE:
	case UM_GOTO_PARAGRAPH:
	case UM_CAN_UNDO:
	case UM_CAN_REDO:
	case UM_UNDO:
	case UM_REDO:
		// Forward to edit pane
		_editPaneWin.SendMsg (msg);
		return true;
	case UM_SCROLL_NOTIFY:
	case UM_REFRESH_UI:
	case UM_DOCPOS_UPDATE:
		// Forward to parent
		_hwndParent.SendMsg (msg);
		return true;
    }
	return false;
}


void EditController::UpdScrollBars ()
{
    // Vertical scroll bars
	Win::UserMessage docVSize (UM_GETDOCDIMENSION, SB_VERT);
	Win::UserMessage winVSize (UM_GETWINDIMENSION, SB_VERT);
	_editPaneWin.SendMsg (docVSize);
    int cLines = docVSize.GetResult ();
	_editPaneWin.SendMsg (winVSize);
    int cLinesPage = winVSize.GetResult ();

	if (cLines == cLinesPage && cLines == 0)
		return;		// No document yet

    if (cLines < cLinesPage)
	{
		int scrollPos = _vScroll.GetPos (cLines, cLinesPage);
		if (scrollPos != 0)
		{
			Win::UserMessage msg (UM_SETDOCPOSITION, SB_VERT, 0);
			_editPaneWin.SendMsg (msg);
		}
        _vScroll.Disable ();
	}
    else
    {
        _vScroll.Init (cLines + 1, cLinesPage, cLinesPage - 1);
		Win::UserMessage hScrollPos (UM_GETSCROLLPOSITION, SB_VERT);
		_editPaneWin.SendMsg (hScrollPos);
		int newPos = hScrollPos.GetResult ();
		_vScroll.SetPosition (newPos);
    }

    // Horizontal scroll bars
	Win::UserMessage docHSize (UM_GETDOCDIMENSION, SB_HORZ);
	Win::UserMessage winHSize (UM_GETWINDIMENSION, SB_HORZ);
	_editPaneWin.SendMsg (docHSize);
    int cChars = docHSize.GetResult ();
	_editPaneWin.SendMsg (winHSize);
    int cCharsPage = winHSize.GetResult ();

    if (cChars <= cCharsPage)
	{
		int scrollPos = _hScroll.GetPos (cChars, cCharsPage);
		if (scrollPos != 0)
		{
			Win::UserMessage msg (UM_SETDOCPOSITION, SB_HORZ, 0);
			_editPaneWin.SendMsg (msg);
		}
        _hScroll.Disable ();
	}
    else
    {
        _hScroll.Init (cChars, cCharsPage, 4); // scroll by 4 chars
		Win::UserMessage vScrollPos (UM_GETSCROLLPOSITION, SB_HORZ);
		_editPaneWin.SendMsg (vScrollPos);
		int newPos = vScrollPos.GetResult ();
		_hScroll.SetPosition (newPos);
    }
}

void EditController::SetScrollBar (int bar, int newPos)
{	
	if (bar == SB_VERT)
	{
		_vScroll.SetPosition (newPos);
	}
	else
	{
		Assert (bar == SB_HORZ);
		_hScroll.SetPosition (newPos);
	}
	Win::UserMessage msg (UM_SETDOCPOSITION, bar, newPos);
    _editPaneWin.SendMsg (msg);	
}

bool EditController::CheckOut ()
{
	if (_isReadOnlyWin)
		return false;

	// Ask parent to check-out file displayed by this window
	Win::UserMessage msg (UM_CHECK_OUT);
	_hwndParent.SendMsg (msg);
	return msg.GetResult () != 0;
}

void EditController::SynchScroll (int offset, int targetPara)
{
	Win::UserMessage msgScroll (UM_SYNCH_SCROLL, offset, targetPara);
	_editPaneWin.SendMsg (msgScroll);
	Win::UserMessage msgPos (UM_GETSCROLLPOSITION, SB_VERT);
	_editPaneWin.SendMsg (msgPos);
	unsigned int newPos = msgPos.GetResult ();
	_vScroll.SetPosition (newPos);
}
