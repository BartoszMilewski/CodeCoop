//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include <WinLibBase.h>
#include "DropDown.h"

using namespace Win;

// winNotify receives control notifications, parentWin is where the child paints itself,
// rectangle defines drop down size and position inside the parent window
DropDown::DropDown (Win::Dow::Handle winNotify, int id, Win::Dow::Handle parentWin)
	: _winNotify (winNotify),
	  _edit (0),
	  _id (id),
	  _dropDownListClosed (true),
	  _selectionValid (false)
{
	ComboBoxMaker comboMaker (_winNotify, id, ComboBox::Style::DropDown);
	comboMaker.Style () << Win::Style::Ex::ClientEdge;
	Reset (comboMaker.Create (parentWin));
	// Get the combo box edit window handle
	::EnumChildWindows (ToNative (), DropDown::FindEditWindow, reinterpret_cast<LPARAM>(&_edit));
	if (_edit.IsNull ())
		throw Win::Exception ("Cannot create drop down control");
	// Change the window procedure for edit control to
	// DropDownWndProc (subclass edit window)
	_orgEditWndProc = _edit.GetLong<WNDPROC> (GWL_WNDPROC);
	_edit.SetLong<WNDPROC> (DropDown::DropDownWndProc, GWL_WNDPROC);
	_edit.SetLong<DropDown *> (this);
}

DropDown::~DropDown ()
{
	// Stop subclassing combo box edit control
	_edit.SetLong<WNDPROC> (_orgEditWndProc, GWL_WNDPROC);
	_edit.SetLong<DropDown *> (0);
}

void DropDown::OnEnter ()
{
	if (_selectionValid)
		return;		// ENTER on the drop down list
	// ENTER in the edit control -- translate to CBN_SELENDOK if necessary
	if (GetEditTextLength () > 0)
	{
		// Notify parent about New selection from
		// the edit control.
		_winNotify.PostMsg (WM_COMMAND, MAKEWPARAM (_id, CBN_SELENDOK), reinterpret_cast<LPARAM>(ToNative ()));
	}
}

// Returns true if selection is ready for processing
bool DropDown::OnNotify (unsigned notifyCode, bool & selectionFromDropDownList)
{
	selectionFromDropDownList = false;
	switch (notifyCode)
	{
	case CBN_DROPDOWN:
		_dropDownListClosed = false;
		break;
	case CBN_EDITCHANGE:
		_selectionValid = false;	// User types text in the edit box
		break;
	case CBN_SELCHANGE:
		if (_dropDownListClosed)
		{
			// User have selected item from the drop down list by clicking on it
			selectionFromDropDownList = true;
			return true;	// Process selection
		}
		else
		{
			_selectionValid = true;	// User browses drop down list using keyboard
		}
		break;
	case CBN_SELENDOK:
		// Selection process ends -- by mouse click or by keyboard ENTER
		if (_selectionValid || !_dropDownListClosed)
		{
			// Display selected item in the edit window
			int selIdx = GetSelectionIdx ();
			std::string selection = RetrieveText (selIdx);
			SetEditText (selection.c_str ());
		}
		else
		{
			// Process text in the edit box - send by subclased edit control
			return true;
		}
		break;
	case CBN_SELENDCANCEL:
		_selectionValid = false;
		break;
	case CBN_CLOSEUP:
		_dropDownListClosed = true;
		selectionFromDropDownList = _selectionValid;
		return _selectionValid;
	}
	return false;
}

BOOL CALLBACK DropDown::FindEditWindow (HWND hwnd, LPARAM lParam)
{
	char className [32];
	if (::GetClassName (hwnd, className, sizeof (className)) != 0)
	{
		if (_stricmp (className, "EDIT") == 0)
		{
			HWND * pEditHwnd = reinterpret_cast<HWND *>(lParam);
			*pEditHwnd = hwnd;
			// Stop EnumChildwindows
			return FALSE;
		}
	}
	// Continue EnumChildWindows
	return TRUE;
}

LRESULT CALLBACK DropDown::DropDownWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Win::Dow::Handle win (hwnd);
	DropDown * pDropDown = win.GetLong<DropDown *> ();
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN)
		{
			pDropDown->OnEnter ();
		}
		break;
	case WM_CHAR:
		// If we don't ignore this, Windows will beep.
		if (wParam == VK_RETURN)
			return 0;
	}
	return pDropDown->CallOrgWndProc (hwnd, msg, wParam, lParam);
}
