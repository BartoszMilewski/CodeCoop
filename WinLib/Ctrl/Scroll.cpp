//
// Reliable Software (c) 1997, 98, 99, 2000
//
#include <WinLibBase.h>
#include "Scroll.h"

int TxtScrollBar::Command (int code, int newPos)
{
    int cLines;
    int cPageLines;
    int iPos = GetPos (cLines, cPageLines);
	int lastLine = cLines - cPageLines;
	if (lastLine != 0)
		lastLine++;
    switch (code)
    {
        case SB_TOP:
            iPos = 0;
            break;
        case SB_BOTTOM:
            iPos = lastLine;
            break;
        case SB_LINEUP:
            iPos--;
            if (iPos < 0)
                iPos = 0;
            break;
        case SB_PAGEUP:
            iPos -= _cPageIncr;
            if (iPos < 0)
                iPos = 0;
            break;
        case SB_LINEDOWN:
            iPos++;
            if (iPos > lastLine)
                iPos = lastLine;
            break;
        case SB_PAGEDOWN:
            iPos += _cPageIncr;
            if (iPos > lastLine)
                iPos = lastLine;
            break;
        case SB_THUMBTRACK:
			iPos = GetTrackPos ();	// Get tracking pos from the scroll bar
			break;
    }
    SetPosition (iPos);
    return iPos;
}

bool TxtScrollBar::IsScrollKey (Win::Dow::Handle hwnd, int vkey)
{
    // Revisit: lParam should be window handle for non-standard scroll bars
    switch (vkey)
    {
    case VK_HOME:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_TOP, 0);
        return true;
    case VK_END:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_BOTTOM, 0);
        return true;
    case VK_PRIOR:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_PAGEUP, 0);
        return true;
    case VK_NEXT:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_PAGEDOWN, 0);
        return true;
    case VK_UP:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_LINEUP, 0);
        return true;
    case VK_DOWN:
        SendMessage (hwnd.ToNative (), WM_VSCROLL, SB_LINEDOWN, 0);
        return true;
    case VK_LEFT:
        SendMessage (hwnd.ToNative (), WM_HSCROLL, SB_PAGELEFT, 0);
        return true;
    case VK_RIGHT:
        SendMessage (hwnd.ToNative (), WM_HSCROLL, SB_PAGERIGHT, 0);
        return true;
    default:
        return false;
    }
}