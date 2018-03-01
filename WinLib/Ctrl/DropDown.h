#if !defined (DROPDOWN_H)
#define DROPDOWN_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include <Ctrl/ComboBox.h>

namespace Win
{
	// This is combo box with subclassed edit control
	// Revisit: use sub-controller
	class DropDown : public Win::ComboBox
	{
	public:
		// winNotify receives control notifications, parentWin is where the child paints itself,
		// rectangle defines drop down size and position inside the parent window
		DropDown (Win::Dow::Handle winNotify, int id, Win::Dow::Handle parentWin);
		~DropDown ();

		void OnEnter ();
		bool OnNotify (unsigned notifyCode, bool & isSelection);
		static bool GotFocus (int notifyCode) { return notifyCode == CBN_SETFOCUS; }
		static bool LostFocus (int notifyCode) { return notifyCode == CBN_KILLFOCUS; }
		LRESULT CallOrgWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			return ::CallWindowProc (_orgEditWndProc, hwnd, msg, wParam, lParam);
		}

	private:
		static BOOL CALLBACK FindEditWindow (HWND hwnd, LPARAM lParam);
		static LRESULT CALLBACK DropDownWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		Win::Dow::Handle	_winNotify;
		Win::Dow::Handle	_edit;
		int					_id;
		WNDPROC				_orgEditWndProc;
		bool				_dropDownListClosed;
		bool				_selectionValid;
	};
}

#endif
