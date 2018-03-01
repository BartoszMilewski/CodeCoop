//----------------------------------
// (c) Reliable Software 2002 - 2005
//----------------------------------

#include <WinLibBase.h>
#include "Controller.h"

#include <Win/Notification.h>
#include <Win/Keyboard.h>
#include <Com/DragDrop.h>
#include <Graph/Canvas.h>
#include <Sys/WheelMouse.h>
#include <Sys/SharedMem.h>
#include <Ctrl/Messages.h>
#include <Ctrl/Output.h>
#include <Win/OwnerDraw.h>
#include <Win/Atom.h>
#include <Win/AppCmdHandler.h>

class SameControl
{
public:
	SameControl (Win::Dow::Handle winParent, unsigned ctrlId)
		: _parent (winParent), _id (ctrlId)
	{}
	bool operator () (OwnerDraw::Handler * handler)
	{
		return handler->CtrlId () == _id && handler->Parent () == _parent;
	}
private:
	Win::Dow::Handle _parent;
	int _id;
};

void Win::Controller::Attach (Win::Dow::Handle win, Win::Controller * ctrl)
{
	ctrl->SetWindowHandle (win);
	win.SetLong<Controller *> (ctrl);
}

void Win::Controller::AddDrawHandler (OwnerDraw::Handler * handler)
{
	// add to list, no duplicates
	HandlerIter it = std::find_if (_drawHandlers.begin (), 
								   _drawHandlers.end (), 
								   SameControl (handler->Parent (), handler->CtrlId ()));
	if (it != _drawHandlers.end ())
		*it = handler;
	else
		_drawHandlers.push_back (handler);
}

void Win::Controller::RemoveDrawHandler (Win::Dow::Handle winParent, unsigned ctrlId) throw ()
{
	_drawHandlers.remove_if (SameControl (winParent, ctrlId));
}

bool Win::Controller::OnItemDraw (OwnerDraw::Item & draw, unsigned ctrlId) throw ()
{
	Assert (ctrlId == draw.CtrlId ());
	HandlerIter it = std::find_if (_drawHandlers.begin (), 
								   _drawHandlers.end (), 
								   SameControl (draw.Window ().GetParent (), ctrlId));
	if (it != _drawHandlers.end ())
	{
		OwnerDraw::Handler * handler = *it;
		return handler->Draw (draw);
	}
	return false;
}

bool Win::Controller::OnAppCommand (unsigned cmd, unsigned device, unsigned virtKeys, Win::Dow::Handle window)
{
	AppCmdHandler * handler = GetAppCmdHandler ();
	if (handler != 0)
	{
		switch (cmd)
		{
		case APPCOMMAND_BROWSER_BACKWARD:
			return handler->OnBrowserBackward ();
		case APPCOMMAND_BROWSER_FORWARD:
			return handler->OnBrowserForward ();
		// to be implemented as needed
		case APPCOMMAND_BROWSER_REFRESH:
		case APPCOMMAND_BROWSER_STOP:
		case APPCOMMAND_BROWSER_SEARCH:
		case APPCOMMAND_BROWSER_FAVORITES:
		case APPCOMMAND_BROWSER_HOME:
		case APPCOMMAND_VOLUME_MUTE:
		case APPCOMMAND_VOLUME_DOWN:
		case APPCOMMAND_VOLUME_UP:
		case APPCOMMAND_MEDIA_NEXTTRACK:
		case APPCOMMAND_MEDIA_PREVIOUSTRACK:
		case APPCOMMAND_MEDIA_STOP:
		case APPCOMMAND_MEDIA_PLAY_PAUSE:
		case APPCOMMAND_LAUNCH_MAIL:
		case APPCOMMAND_LAUNCH_MEDIA_SELECT:
		case APPCOMMAND_LAUNCH_APP1:
		case APPCOMMAND_LAUNCH_APP2:
		case APPCOMMAND_BASS_DOWN:
		case APPCOMMAND_BASS_BOOST:
		case APPCOMMAND_BASS_UP:
		case APPCOMMAND_TREBLE_DOWN:
		case APPCOMMAND_TREBLE_UP:
		default:
			break;
		}
	}
	return false;
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
		if (OnVScroll (LOWORD (wParam), HIWORD (wParam), reinterpret_cast<HWND> (lParam)))
			return true;
		break;
	case WM_HSCROLL:
		if (OnHScroll (LOWORD (wParam), HIWORD (wParam), reinterpret_cast<HWND> (lParam)))
			return true;
		break;
	case WM_CLOSE:
		if (OnClose ())
			return true;
		break;
	case WM_ERASEBKGND:
        if (OnEraseBkgnd(HDC(wParam)))
        {
            result = TRUE;
            return true;
        }
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
			int cmdId = LOWORD (wParam);
			bool isAccel = HIWORD (wParam) == 1;
			if (OnCommand (cmdId, isAccel))
				return true;
		}
		else
		{
			Win::Dow::Handle winCtrl = reinterpret_cast<HWND>(lParam);
			unsigned ctrlId = LOWORD (wParam);
			unsigned notifyCode = HIWORD (wParam);
			Control::Handler * handler = GetControlHandler (winCtrl, ctrlId);
			if (handler != 0)
			{
				return handler->OnControl (ctrlId, notifyCode);
			}
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR * hdr = reinterpret_cast<NMHDR *>(lParam);
			Notify::Handler * pHandler = GetNotifyHandler (hdr->hwndFrom, hdr->idFrom);
			if (pHandler != 0)
			{
				if (pHandler->OnNotify (hdr, result))
				{
					_h.SetLong (result, DWL_MSGRESULT);
					return true;
				}
			}
			break;
		}
	case WM_INITMENUPOPUP:
		if (HIWORD (lParam) == 0)
		{
			if (OnInitPopup (reinterpret_cast<HMENU>(wParam), LOWORD (lParam)))
				return true;
		}
		else // system menu
		{
			if (OnInitSystemPopup (reinterpret_cast<HMENU>(wParam), LOWORD (lParam)))
				return true;
		}
		break;
	case WM_CONTEXTMENU:
		{
			POINTS p = MAKEPOINTS (lParam);
			if (OnContextMenu (reinterpret_cast<HWND>(wParam), p.x, p.y))
				return true;
		}
		break;
	case WM_EXITMENULOOP:
		if (OnExitMenuLoop (static_cast<BOOL> (wParam) != FALSE))
			return true;
		break;
	case WM_MENUSELECT:
		{
			unsigned int value;
			if (static_cast<unsigned int>(HIWORD (wParam)) == 0xffff && lParam == 0)
				value = 0xf0000000;
			else
				value = static_cast<unsigned int>(HIWORD (wParam));
			Menu::State state (value);
			if (state.IsPopup ())
			{
				if (OnPopupSelect (LOWORD (wParam), state, reinterpret_cast<HMENU>(lParam)))
					return true;
			}
			else if (state.IsSysMenu ())
			{
				if (OnSysMenuSelect (LOWORD (wParam), state, reinterpret_cast<HMENU>(lParam)))
					return true;
			}
			else
			{
				if (OnMenuSelect (LOWORD (wParam), state, reinterpret_cast<HMENU>(lParam)))
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
	case WM_MOUSEWHEEL:
		if (OnWheelMouse (wParam))
			return true;
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
	case WM_DROPFILES:
		{
			Win::FileDropOwner fileDrop (wParam);
			if (OnDropFiles (fileDrop))
				return true;
		}
		break;
	case WM_KEYDOWN:
		{
			Keyboard::Handler * pHandler = GetKeyboardHandler ();
			if (pHandler != 0)
			{
				if (pHandler->OnKeyDown (wParam, lParam))
					return true;
			}
		}
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
	case WM_DDE_INITIATE:
		{
			Win::Dow::Handle client (reinterpret_cast<HWND>(wParam));
			Win::GlobalAtom::String app (LOWORD (lParam));
			Win::GlobalAtom::String topic (HIWORD (lParam));
			if (OnDdeInitiate (client, app, topic))
				return true;
		}
		break;
	case WM_DRAWITEM:
		{
			OwnerDraw::Item draw (reinterpret_cast<DRAWITEMSTRUCT*> (lParam));
			if (wParam == 0)
			{
				Assert (draw.ControlType () == OwnerDraw::Menu);
				int menuId = draw.ItemId ();
				if (OnMenuDraw (draw))
				{
					result = TRUE;
					return true;
				}
			}
			else
			{
				if (OnItemDraw (draw, wParam))
				{
					result = TRUE;
					return true;
				}
			}
		}
		break;
	case WM_MEASUREITEM:
		{
			// Revisit
			MEASUREITEMSTRUCT *pmis = reinterpret_cast<MEASUREITEMSTRUCT*> (lParam);

			if (pmis->CtlType == ODT_LISTBOX)
			{
				Assert(wParam == pmis->CtlID);
				pmis->itemHeight = 0;
				if (OnMeasureListBoxItem(pmis->CtlID, pmis->itemID, pmis->itemHeight))
				{
					Assert(pmis->itemHeight != 0);
					result = TRUE;
					return true;
				}
			}
		}
		break;
	case WM_CTLCOLOREDIT:
		{
			Brush::Handle hbrResult(0);
			if (OnPreDrawEdit(reinterpret_cast<HWND> (lParam), reinterpret_cast<HDC> (wParam), hbrResult))
			{
				Assert(hbrResult != 0);
				result = reinterpret_cast<LRESULT> (hbrResult.ToNative ());
				return true;
			}
		}
		break;
	case WM_CTLCOLORSTATIC:
		{
			Brush::Handle hbrResult(0);
			if (OnPreDrawStatic(reinterpret_cast<HWND> (lParam), reinterpret_cast<HDC> (wParam), hbrResult))
			{
				Assert(hbrResult != 0);
				result = reinterpret_cast<LRESULT> (hbrResult.ToNative ());
				return true;
			}
		}
		break;
	case WM_CTLCOLORLISTBOX:
		{
			Brush::Handle hbrResult(0);
			if (OnPreDrawListBox(reinterpret_cast<HWND> (lParam), reinterpret_cast<HDC> (wParam), hbrResult))
			{
				Assert(hbrResult != 0);
				result = reinterpret_cast<LRESULT> (hbrResult.ToNative ());
				return true;
			}
		}
		break;
	case WM_SETFONT:
		{
			Font::Handle font(reinterpret_cast<HFONT> (wParam));
			if (OnSetFont(font, LOWORD(lParam) != 0))
			{
				//	WM_SETFONT doesn't return a meaningful value
				return true;
			}
		}
		break;
	case WM_ENTERIDLE:
		{
			if (wParam == MSGF_DIALOGBOX)
			{
				if (OnDialogIdle((HWND) lParam))
				{
					return true;
				}
			}
			else if (wParam == MSGF_MENU)
			{
				//	unused
			}
		}
		break;
	case WM_APPCOMMAND:
		{
			unsigned cmd  = GET_APPCOMMAND_LPARAM(lParam);
			unsigned device = GET_DEVICE_LPARAM(lParam);
			unsigned virtKeys = GET_KEYSTATE_LPARAM(lParam);		
			if (OnAppCommand (cmd, device, virtKeys, Win::Dow::Handle (HWND (wParam))))
				return true;
		}
		break;
	case WM_TCARD:
		{
			if (OnTrainingCard (wParam, lParam))
			{
				return true;
			}
		}
		break;
	default:
		if (WM_USER <= message && message < 0x8000)
		{
			UserMessage msg (message - WM_USER, wParam, lParam);
			if (msg.GetMsg () == UM_INTERPROCESS_PACKAGE)
			{
				// Handle RS Library interprocess message
				Assert (0xc000 <= wParam && wParam <= 0xffff);
				GlobalAtom::String memName (static_cast<unsigned>(lParam));
				SharedMem buf;
				buf.Open (memName, 0, true);	//Size unknown, quiet open
				unsigned int errCode = 0;
				if (!buf.IsOk ())
					errCode = ::GetLastError ();
				if (OnInterprocessPackage (wParam, &buf[0], errCode, result))
					return true;
			}
			else if (OnUserMessage (msg))
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
