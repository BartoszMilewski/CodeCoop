// -----------------------------------
// (c) Reliable Software, 1999 -- 2002
// -----------------------------------

#include <WinLibBase.h>
#include "MsgLoop.h"

namespace Win
{

	int MessagePrepro::Pump ()
	{
		MSG  msg;
		int status;
		while ((status = ::GetMessage (&msg, 0, 0, 0 )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows message loop");
			if (!TranslateDialogMessage (msg))
			{			
				if (_accel == 0 || !_accel->Translate (msg)) 
				{
					::TranslateMessage (&msg);
					::DispatchMessage (&msg);
				}				
			}
		}
		return msg.wParam;
	}

	bool MessagePrepro::TranslateDialogMessage (MSG & msg)
	{
		if (_winDlg.IsNull ())
			return false;
		if (!_hDlgAccel.IsNull () 
			&& ::TranslateAccelerator (_winDlgParent.ToNative (), _hDlgAccel.ToNative (), &msg))
		{
			return true;
		}
		return ::IsDialogMessage (_winDlg.ToNative (), &msg) != FALSE;
	}

	int MessagePrepro::PumpHidden (Win::Dow::Handle hwnd)
	{
		MSG  msg;
		int status;
		while ((status = ::GetMessage (&msg, hwnd.ToNative (), 0, 0 )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows message loop");
			if (msg.message == _breakMsg)
				break;
			::DispatchMessage (&msg);
		}
		return msg.wParam;
	}

	int MessagePrepro::Pump (Win::Dow::Handle hwnd)
	{
		MSG  msg;
		int status;
		while ((status = ::GetMessage (&msg, hwnd.ToNative (), 0, 0 )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows message loop");
			if (_winDlg.IsNull () || !::IsDialogMessage (_winDlg.ToNative (), &msg))
			{
				if (_accel == 0 || !_accel->Translate (msg)) 
				{
					::TranslateMessage (&msg);
					::DispatchMessage (&msg);
				}
			}
		}
		return msg.wParam;
	}

	// Peek at the queue and process any messages
	void MessagePrepro::PumpPeek ()
	{
		MSG  msg;
		int status;
		while ((status = ::PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows peek message loop");
			if (_winDlg.IsNull () || !::IsDialogMessage (_winDlg.ToNative (), &msg))
			{
				if (_accel == 0 || !_accel->Translate (msg)) 
				{
					::TranslateMessage (&msg);
					::DispatchMessage (&msg);
				}
			}
		}
	}

	// Peek at the queue and process only messages
	// to the specified window. Don't translate accellerators
	void MessagePrepro::PumpPeek (Win::Dow::Handle hwnd)
	{
		MSG  msg;
		int status;
		while ((status = ::PeekMessage (&msg, hwnd.ToNative (), 0, 0, PM_REMOVE )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows peek message loop");
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
		}
	}

	// Peek at the queue and process only messages
	// to the currently active modeless dialog. Don't translate accellerators
	void MessagePrepro::PumpDialog ()
	{
		if (!_winDlg.IsNull ())
		{
			MSG  msg;
			int status;
			while ((status = ::PeekMessage (&msg, _winDlg.ToNative (), 0, 0, PM_REMOVE )) != 0)
			{
				if (status == -1)
					throw Win::Exception ("Error in the dialog peek message loop");
				if (!::IsDialogMessage (_winDlg.ToNative (), &msg))
				{
					::TranslateMessage (&msg);
					::DispatchMessage (&msg);
				}
			}
		}
	}

// Modal pump

	int ModalMessagePrepro::Pump (Win::Dow::Handle hwnd)
	{
		MSG  msg;
		int status;
		while ((status = ::GetMessage (&msg, hwnd.ToNative (), 0, 0 )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows modal message loop");
			if (msg.message == _breakMsg)
				break;
			switch (msg.message)
			{
			// Filter out the following messages:
			// Mouse
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEACTIVATE:
			// Revisit: WM_MOUSEHOVER not available in NT 4.0 even the docs says otherwise ?!?
			//case WM_MOUSEHOVER:
			case WM_MOUSEMOVE:
			// Revisit: WM_MOUSEWHEEL not available in NT 4.0 even the docs says otherwise ?!?
			//case WM_MOUSEWHEEL:
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			// Keyboard
			case WM_CHAR:
			case WM_CHARTOITEM:
			case WM_DEADCHAR:
			case WM_GETHOTKEY:
			case WM_HOTKEY:
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SETHOTKEY:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_SYSCHAR:
			case WM_SYSDEADCHAR:
			case WM_VKEYTOITEM:
				break;
			default:
				// Dispatch all other messages
				::TranslateMessage (&msg);
				::DispatchMessage (&msg);
				break;
			}
		}
		return msg.wParam;
	}

	int ModalMessagePrepro::Pump ()
	{
		MSG  msg;
		int status;
		while ((status = ::GetMessage (&msg, 0, 0, 0 )) != 0)
		{
			if (status == -1)
				throw Win::Exception ("Error in the Windows modal message loop (all windows)");
			if (msg.message == _breakMsg)
				break;
			switch (msg.message)
			{
			// Filter out the following messages:
			// Mouse
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEACTIVATE:
			// Revisit: WM_MOUSEHOVER not available in NT 4.0 even the docs says otherwise ?!?
			//case WM_MOUSEHOVER:
			case WM_MOUSEMOVE:
			// Revisit: WM_MOUSEWHEEL not available in NT 4.0 even the docs says otherwise ?!?
			//case WM_MOUSEWHEEL:
			case WM_RBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			// Keyboard
			case WM_CHAR:
			case WM_CHARTOITEM:
			case WM_DEADCHAR:
			case WM_GETHOTKEY:
			case WM_HOTKEY:
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SETHOTKEY:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_SYSCHAR:
			case WM_SYSDEADCHAR:
			case WM_VKEYTOITEM:
				break;
			default:
				// Dispatch all other messages
				::TranslateMessage (&msg);
				::DispatchMessage (&msg);
				break;
			}
		}
		return msg.wParam;
	}
}

