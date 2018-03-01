//
// (c) Reliable Software, 1997
//

#include <WinLibBase.h>
#include "MarginCtrl.h"

LRESULT CALLBACK WndProcMargin (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Win::Dow::Handle win (hwnd);
    MarginController * pCtrl = win.GetLong<MarginController *> ();

    switch (message)
    {
    case WM_CREATE:
        try
        {
            pCtrl = new MarginController (hwnd, reinterpret_cast<CREATESTRUCT *>(lParam));
            win.SetLong<MarginController *> (pCtrl);
        }
        catch (...)
        {
            return -1;
        }
        return 0;
    case WM_TIMER:
        pCtrl->Timer ();
        return 0;
    case WM_SIZE:
        pCtrl->Size (LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_LBUTTONDOWN:
        pCtrl->LButtonDown (wParam, MAKEPOINTS (lParam));
        return 0;
    case WM_LBUTTONUP:
        pCtrl->LButtonUp (MAKEPOINTS (lParam));
        return 0;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON)
            pCtrl->LButtonDrag (MAKEPOINTS (lParam));
        return 0;
	case WM_CAPTURECHANGED:
		pCtrl->CaptureChanged ();
		return 0;
    case WM_DESTROY:
        win.SetLong<MarginController *> (0);
        delete pCtrl;
        return 0;
    }
    return DefWindowProc (hwnd, message, wParam, lParam);
}

