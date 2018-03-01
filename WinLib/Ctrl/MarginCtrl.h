#if !defined (MARGINCTRL_H)
#define MARGINCTRL_H
//
// (c) Reliable Software, 1997, 98, 99, 2000
//

#include "Messages.h"

#include <Sys/Timer.h>
#include <Win/Message.h>

class MarginController
{
public:
    MarginController (HWND hwnd, CREATESTRUCT * pCreat)
        : _hwnd (hwnd),
		  _hwndParent (pCreat->hwndParent),
		  _timer (1)
    {
		_timer.Attach (_hwnd);
	}
    Win::Dow::Handle Hwnd () const { return _hwnd; }
    void Size (int cx, int cy) { _cy = cy; }
    void Timer ()
    {
        _timer.Kill ();
        if (_hwnd.HasCapture ())
        {
            LButtonDrag (_mousePoint);
        }
    }

    void LButtonDown (WPARAM flags, POINTS pt)
    {
        _hwnd.CaptureMouse ();
		Win::UserMessage msg (UM_STARTSELECTLINE, flags, pt.y);
        _hwndParent.SendMsg (msg);
    }

    void LButtonUp (POINTS pt)
    {
        _mousePoint = pt;
		// Calling ReleaseCapture will send us the WM_CAPTURECHANGED
        _hwnd.ReleaseMouse ();
    }
    void LButtonDrag (POINTS pt)
    {
        _mousePoint = pt;
		Win::UserMessage msg (UM_SELECTLINE, 0, pt.y);
        _hwndParent.SendMsg (msg);
        if (pt.y < 0 || pt.y > _cy)
            _timer.Set (100);
    }
	void CaptureChanged ()
	{
		// End drag selection -- for whatever reason
		Win::UserMessage msg (UM_ENDSELECTLINE, 0, _mousePoint.y);
		_hwndParent.SendMsg (msg);
	}
private:
    Win::Dow::Handle    _hwnd;
    Win::Dow::Handle    _hwndParent;
    int         _cy;
    // Dragging support
    POINTS      _mousePoint;
	Win::Timer	_timer;
};

#endif
