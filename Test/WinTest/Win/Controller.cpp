//
// (c) Reliable Software 2002 -- 2003
//
#include <Win/Controller.h>
#include <Win/Win.h>
#include <Win/Output.h>

void Win::Controller::Attach (Win::Dow::Handle win, Win::Controller * ctrl)
{
	ctrl->SetWindowHandle (win.ToNative ());
	win.SetLong<Controller *> (ctrl);
}

bool Win::Controller::Dispatch (UINT message, WPARAM wParam, LPARAM lParam, LRESULT & result)
{
    switch (message)
    {
	case WM_CREATE:
		{
			// Make window controller operational
			bool success = false;
			CreateData const * creation = 
				reinterpret_cast<CreateData const *> (lParam);
			if (OnCreate (creation, success))
			{
				result = success? 0: -1;
				return true;
			}
			break;
		}
	case WM_INITDIALOG:
		if (OnInitDialog ())
			return true;
		break;
	case WM_NCDESTROY:
		if (MustDestroy ())
			delete this;
		break;	
    case WM_DESTROY:
        // We're no longer on screen
        OnDestroy ();
        return true;
	case WM_ENDSESSION:
		if (OnShutdown (static_cast<BOOL> (wParam) != FALSE, lParam == ENDSESSION_LOGOFF))
			return true;
		break;
	case WM_SETTINGCHANGE:
		{
			SystemWideFlags flags (wParam);
			if (OnSettingChange (flags, reinterpret_cast<char const *>(lParam)))
				return true;
		}
		break;
	case WM_ACTIVATEAPP:
		{
			bool isActive = wParam != FALSE;
			unsigned long threadId = lParam;
			if (isActive)
			{
				if (OnActivateApp (threadId))
					return true;
			}
			else
			{
				if (OnDeactivateApp (threadId))
					return true;
			}
		}
		break;
	case WM_ACTIVATE:
		{
			ActiveAction action (LOWORD (wParam));
			bool isMinimized = HIWORD (wParam) != FALSE;
			//	wnd is prev or next depending on action
			Win::Dow::Handle wnd (reinterpret_cast<HWND> (lParam));
			if (action.IsInActive ())
			{
				if (OnDeactivate (isMinimized, wnd))
					return true;
			}
			else
			{
				if (OnActivate (action.IsClickActive (), isMinimized, wnd))
					return true;
			}
		}
		break;
	case WM_MOUSEACTIVATE:
		{
			MouseActiveAction & action = reinterpret_cast<MouseActiveAction &> (result);
			HitTest hitTest (LOWORD (lParam));
			if (OnMouseActivate (hitTest, action))
				return true;
		}
		break;
	case WM_SETFOCUS:
		{
			Win::Dow::Handle wndPrev = reinterpret_cast<HWND> (wParam);
			if (OnFocus (wndPrev))
				return true;
		}
		break;
	case WM_KILLFOCUS:
		{
			Win::Dow::Handle wndNext = reinterpret_cast<HWND> (wParam);
			if (OnKillFocus (wndNext))
				return true;
		}
		break;
	case WM_ENABLE:
		{
			bool IsEnabled = (wParam != FALSE);
			if (IsEnabled)
			{
				if (OnEnable())
					return true;
			}
			else
			{
				if (OnDisable())
					return true;
			}
		}
		break;
	case WM_SIZE:
		if (OnSize (LOWORD (lParam), HIWORD (lParam), wParam))
			return true;
		break;
    case WM_VSCROLL:
		if (OnVScroll (LOWORD (wParam), HIWORD (wParam)))
			return true;
		break;
	case WM_HSCROLL:
		if (OnHScroll (LOWORD (wParam), HIWORD (wParam)))
			return true;
		break;
	case WM_CLOSE:
		if (OnClose ())
			return true;
		break;
	case WM_PAINT:
		if (OnPaint ())
			return true;
		break;
	case WM_SHOWWINDOW:
		if (OnShowWindow (wParam != FALSE))
			return true;
		break;
	case WM_COMMAND:
		if (lParam == 0)
		{
			if (OnCommand (LOWORD (wParam), HIWORD (wParam) == 1))
				return true;
		}
		else
		{
			int ctrlId = LOWORD(wParam);
			if (ctrlId == Out::Help)
			{
				if (OnHelp ())
					return true;
			}
			else
			{
				Win::Dow::Handle win = reinterpret_cast<HWND>(lParam);
				if (OnControl (win, ctrlId, HIWORD (wParam)))
					return true;
			}
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINTS p = MAKEPOINTS (lParam);
			KeyState kState (wParam);
			if (OnMouseMove (p.x, p.y, kState))
				return true;
		}
		break;
	case WM_LBUTTONDOWN:
		{
			POINTS p = MAKEPOINTS (lParam);
			KeyState kState (wParam);
			if (OnLButtonDown (p.x, p.y, kState))
				return true;
		}
		break;
	case WM_LBUTTONUP:
		{
			POINTS p = MAKEPOINTS (lParam);
			KeyState kState (wParam);
			if (OnLButtonUp (p.x, p.y, kState))
				return true;
		}
		break;
	case WM_LBUTTONDBLCLK:
		{
			POINTS p = MAKEPOINTS (lParam);
			KeyState kState (wParam);
			if (OnLButtonDblClick (p.x, p.y, kState))
				return true;
		}
		break;
	case WM_CAPTURECHANGED:
		if (OnCaptureChanged (reinterpret_cast<HWND> (lParam)))
			return true;
		break;
	case WM_CHAR:
		if (OnChar (wParam, lParam))
			return true;
		break;
	case WM_CHARTOITEM:
		if (OnCharToItem(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND> (lParam)))
		{
			result = -2;
			return true;
		}
		break;
	case WM_VKEYTOITEM:
		if (OnVKeyToItem(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND> (lParam)))
		{
			result = -2;
			return true;
		}
		break;
	case WM_TIMER:
		if (OnTimer (wParam))
			return true;
		break;
	default:
		if (WM_USER <= message && message < 0x8000)
		{
			UserMessage msg (message - WM_USER, wParam, lParam);
			if (OnUserMessage (msg))
			{
				result = msg.GetResult ();
				return true;
			}
		}
		else if (0xc000 <= message && message <= 0xffff)
		{
			Message msg (message, wParam, lParam);
			if (OnRegisteredMessage (msg))
			{
				result = msg.GetResult ();
				return true;
			}
		}
		break;
    }
	return false;
}
