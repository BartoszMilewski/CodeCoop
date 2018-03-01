#if !defined SCROLL_H
#define SCROLL_H
//
// Reliable Software (c) 1997-2000
//

// This is a vertical scroll bar of a window

class TxtScrollBar
{
public:
    TxtScrollBar (Win::Dow::Handle win)
        : _win (win), _cPageIncr (0)
    {
        _info.cbSize = sizeof (SCROLLINFO);
    }

	operator Win::Dow::Handle () const { return _win; }

	void Init (Win::Dow::Handle win) { _win = win; }
    void Init (int cLines, int cPageLines, int cPageIncr)
    {
		if (cPageLines < 0)// sometime we call this function wich cPageLines == -1,
			cPageLines = 0;//On NT : if nPage < 0 result perhaps be bad		                 
        _cPageIncr = cPageIncr;
        _info.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
        _info.nMin = 0;
        _info.nMax = cLines;
        _info.nPage = cPageLines;
        SetInfo ();
    }

	void Disable ()
	{
		_info.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
		_info.nMin = 0;
		_info.nMax = 0;
		SetInfo ();
	}

    int GetPos (int & cLines, int & cPageLines)
    { 
        _info.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
        GetInfo ();
        cLines = _info.nMax;
        cPageLines = _info.nPage;
        return _info.nPos; 
    }

    int GetTrackPos ()
    {
		// Obtains full 32-bit tracking position
        _info.fMask = SIF_TRACKPOS;
        GetInfo ();
        return _info.nTrackPos; 
    }

    int GetRange ()
    {
        _info.fMask = SIF_RANGE;
        GetInfo ();
        return _info.nMax;
    }

    int GetPage ()
    {
        _info.fMask = SIF_PAGE;
        GetInfo ();
        return _info.nPage;
    }

    virtual void Hide () = 0;
    virtual void Show () = 0;

    // Usage: Command (LOWORD (wParam), HIWORD (wParam));
    // Returns New position
	// Note: SB_THUMBPOSITION not supported -- use SetPosition instead
    int Command (int code, int pos);
    // Call in response to WM_KEYDOWN with wParam
    // Side effect: sends scroll messages to win
    static bool IsScrollKey (Win::Dow::Handle win, int vkey);

    void SetPosition (int iPos)
    {
        _info.fMask = SIF_POS;
        _info.nPos = iPos;
        SetInfo ();
    }
protected:

    virtual void GetInfo () = 0;
    virtual void SetInfo () = 0;

protected:

    Win::Dow::Handle    _win;
    SCROLLINFO  _info;
    int         _cPageIncr;
};

class VTxtScrollBar: public TxtScrollBar
{
public:
    VTxtScrollBar (Win::Dow::Handle win)
        : TxtScrollBar (win)
    {}

    void Show ()
    {
		::ShowScrollBar (_win.ToNative (), SB_VERT, TRUE);
    }
    void Hide ()
    {
		::ShowScrollBar (_win.ToNative (), SB_VERT, FALSE);
    }

protected:
    void GetInfo ()
    {
		::GetScrollInfo (_win.ToNative (), SB_VERT, &_info);
    }

    void SetInfo ()
    {
		::SetScrollInfo (_win.ToNative (), SB_VERT, &_info, TRUE);
    }
};

class HTxtScrollBar: public TxtScrollBar
{
public:
    HTxtScrollBar (Win::Dow::Handle win)
        : TxtScrollBar (win)
    {}
    void Show ()
    {
		::ShowScrollBar (_win.ToNative (), SB_HORZ, TRUE);
    }
    void Hide ()
    {
		::ShowScrollBar (_win.ToNative (), SB_HORZ, FALSE);
    }

protected:
    void GetInfo ()
    {
		::GetScrollInfo (_win.ToNative (), SB_HORZ, &_info);
    }

    void SetInfo ()
    {
		::SetScrollInfo (_win.ToNative (), SB_HORZ, &_info, TRUE);
    }
};

class TxtScrollBarCtrl: public TxtScrollBar
{
public:
    TxtScrollBarCtrl (Win::Dow::Handle win)
        : TxtScrollBar (win)
    {}
    void Show ()
    {
		::ShowScrollBar (_win.ToNative (), SB_CTL, TRUE);
    }
    void Hide ()
    {
		::ShowScrollBar (_win.ToNative (), SB_CTL, FALSE);
    }

protected:
    void GetInfo ()
    {
		::GetScrollInfo (_win.ToNative (), SB_CTL, &_info);
    }

    void SetInfo ()
    {
		::SetScrollInfo (_win.ToNative (), SB_CTL, &_info, TRUE);
    }
};
#endif
