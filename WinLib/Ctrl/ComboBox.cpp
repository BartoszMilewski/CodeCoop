//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include <WinLibBase.h>
#include "ComboBox.h"

#include <StringOp.h>

using namespace Win;

void ComboBox::OnNotify (unsigned int code)
{
	if (_notifyHandler.get () != 0)
	{
		switch (code)
		{
		case CBN_SELENDOK:
			_notifyHandler->OnSelEndOk ();
			break;
		case CBN_CLOSEUP:
			_notifyHandler->OnCloseUp ();
			break;
		case CBN_DBLCLK:
			_notifyHandler->OnDblClick ();
			break;
		case CBN_SELENDCANCEL:
			_notifyHandler->OnSelEndCancel ();
			break;
		case CBN_KILLFOCUS:
			_notifyHandler->OnKillFocus ();
			break;
		case CBN_EDITCHANGE:
			_notifyHandler->OnEditChange ();
			break;
		case CBN_DROPDOWN:
			_notifyHandler->OnDropDown ();
			break;
		case CBN_EDITUPDATE:
			_notifyHandler->OnEditUpdate ();
			break;
		case CBN_SETFOCUS:
			_notifyHandler->OnSetFocus ();
			break;
		case CBN_SELCHANGE:
			_notifyHandler->OnSelChange ();
			break;
		}
	}
}

void ComboBox::Display (char const * info)
{
	// Make the info the combobox current selection
	SendMsg (CB_RESETCONTENT, 0, 0);
	SendMsg (CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(info));
	SendMsg (CB_SETCURSEL, 0, 0);
}

long ComboBox::GetHeight () const
{
	Win::Rect rect;
	GetWindowRect (rect);
	return rect.bottom - rect.top;
}

std::string ComboBox::RetrieveEditText () const
{
	std::vector<char> text;
	std::string result;
    int len = GetEditTextLength ();
    if (len > 0)
    {
		text.resize (len + 1);
		SendMsg (WM_GETTEXT, len + 1, reinterpret_cast<LPARAM>(&text [0]));
		result.assign (&text [0]);
    }
	return result;
}

std::string ComboBox::RetrieveTrimmedEditText () const
{
	TrimmedString str (RetrieveEditText ());
	return str;
}

std::string ComboBox::RetrieveText (int idx) const
{
	std::vector<char> text;
	std::string result;
    int len = GetTextLength (idx);
    if (len > 0)
    {
		text.resize (len + 1);
		SendMsg (CB_GETLBTEXT, idx, reinterpret_cast<LPARAM>(&text [0]));
		result.assign (&text [0]);
    }
	return result;
}

int ComboBox::RowsToHeight(int nRows) const
{
	Win::Rect rc;
	GetWindowRect(rc);

//	ComboBoxInfo cbi(this);
//	Win::WindowInfo wiListBox(cbi.hwndList);

	//	We'd like to use wiListBox.VerticalBorderSize() (instead of '1'),
	//  but WindowInfo isn't available on Windows 95 or Windows NT 4.0 pre SP3
	return rc.Height() + 
		   GetItemHeight() * nRows +
//		   wiListBox.VerticalBorderSize() * 2;
		   1 * 2;
}

#if defined(TESTING)
//	Uses CombBoxInfo (which is not supported on Windows 95)

#if !defined NDEBUG
std::ostream& Win::operator<<(std::ostream& os, ComboBox& cbo)
{
	os << "Win::ComboBox Information for " << std::hex << (HWND) cbo << std::endl;
	Win::ComboBox::ComboBoxInfo cbi(&cbo);
	os << std::dec;
	os << "  rcItem: " << cbi.rcItem << std::endl;
	os << "  rcButton: " << cbi.rcButton << std::endl;
	os << "stateButton " << cbi.stateButton << std::endl;
	os << std::hex;
	Win::ComboBox::WindowInfo wiCombo(Win::ComboBox(cbi.hwndCombo));
	os << "hwndCombo: " << wiCombo << std::endl;
	if (wiCombo.IsDropDown())
	{
		os << "hwndItem: " << WindowInfo(cbi.hwndItem) << std::endl;
	}
	os << "hwndList: " << WindowInfo(cbi.hwndList) << std::endl;
	return os;
}
#endif

#endif
