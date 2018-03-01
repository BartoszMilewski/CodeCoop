//
// (c) Reliable Software 1997-2002
//
#include <Win/Win.h>
#include <Win/Geom.h>
#include <Win/Message.h>
#include <Win/Procedure.h>
#include <Win/Controller.h>

using namespace Win;

// Revisit: This should be in a header file, but
// C7 doesn't implement specializations correctly
// Specialization of Dispose for HWND
template<>
void Win::Disposal<Win::window_handle>::Dispose (Win::window_handle h) throw ()
{
	::DestroyWindow (h.ToNative ());
}

void Win::Quit (int errorCode)
{
	::PostQuitMessage (errorCode);
}

void Dow::Handle::SendMsg (Message & msg) const
{
	assert (!IsNull ());
	LRESULT result = ::SendMessage (_h, msg.GetMsg (), msg.GetWParam (), msg.GetLParam ());
	msg.SetResult (result);
}

bool Dow::Handle::PostMsg (Message const & msg) const
{
	assert (!IsNull ());
	return ::PostMessage (_h, msg.GetMsg (), msg.GetWParam (), msg.GetLParam ()) != FALSE;
}

void Dow::Handle::ClientToScreen (Win::Point & pt) const
{
	::ClientToScreen (_h, &pt);
}

void Dow::Handle::Invalidate (Win::Rect const & rect)
{
	::InvalidateRect (_h, &rect, TRUE);
}

void Dow::Handle::GetClientRect (Win::Rect & rect) const
{
	::GetClientRect (_h, &rect);
}

void Dow::Handle::GetWindowRect (Win::Rect & rect) const
{
	::GetWindowRect (_h, &rect);
}

void Dow::Handle::Scroll (Win::Rect & rect, int xAmount, int yAmount)
{
	::ScrollWindow (_h, xAmount, yAmount, &rect, 0);
}

void Dow::Handle::SubClass (SubController * subCtrl)
{
	// get previous window procedure and controller (if any)
	ProcPtr prevProc = GetLong<ProcPtr>(GWL_WNDPROC);

	Controller * prevCtrl = GetLong<Controller *> ();
	// remember them in the new controller
	subCtrl->Init (_h, prevProc, prevCtrl);
	// attach new controller to window
	SetLong<SubController *>(subCtrl);
	// attach new window procedure to window
	SetLong(reinterpret_cast<long> (SubProcedure), GWL_WNDPROC);
}

void Dow::Handle::UnSubClass ()
{
	// Get the current subclass controller
	SubController * pCtrl = GetLong<SubController *>();
	// restore previous window procedure and controller (if any)
	SetLong<Win::ProcPtr> (pCtrl->GetPrevProc(), GWL_WNDPROC);
	SetLong<Win::Controller *> (pCtrl->GetPrevController ());
}

//-----------------------
// Placement methods
//-----------------------

Placement::Placement ()
{
	memset (this, 0, sizeof (WINDOWPLACEMENT));
	_pl.length = sizeof (WINDOWPLACEMENT);
}

Placement::Placement (Dow::Handle win)
{
	Init (win);
}

void Placement::Init (Dow::Handle win)
{
	assert (!win.IsNull ());
	_pl.length = sizeof (WINDOWPLACEMENT);
	::GetWindowPlacement (win.ToNative (), &_pl);
}

int  Placement::GetFlags () const
{
	return _pl.flags;
}

bool Placement::IsMaximized () const
{
	return _pl.showCmd == SW_SHOWMAXIMIZED;
}

bool Placement::IsMinimized () const
{
	return _pl.showCmd == SW_SHOWMINIMIZED;
}

bool Placement::IsHidden () const
{
	return _pl.showCmd == SW_HIDE;
}

void  Placement::GetMaxCorner (Win::Point & pt) const
{
	pt = _pl.ptMaxPosition;
}

void  Placement::GetMinCorner (Win::Point & pt) const
{
	pt = _pl.ptMinPosition;
}

void  Placement::GetRect (Win::Rect & rect) const
{
	rect = _pl.rcNormalPosition;
}

void Placement::SetFlags (int flags)
{
	_pl.flags = flags;
}

void Placement::SetNormal ()
{
	_pl.showCmd = SW_SHOWNORMAL;
}

void Placement::SetMaximized ()
{
	_pl.showCmd = SW_SHOWMAXIMIZED;
}

void Placement::SetMinimized ()
{
	_pl.showCmd = SW_SHOWMINIMIZED;
}

void Placement::SetHidden ()
{
	_pl.showCmd = SW_HIDE;
}

void Placement::SetRect (Win::Rect & rect)
{
	_pl.rcNormalPosition = rect;
}

void Placement::SetMinCorner(Win::Point & pt)
{
	_pl.ptMinPosition = pt;
}

void Placement::SetMaxCorner(Win::Point & pt)
{
	_pl.ptMaxPosition = pt;
}

void Placement::CombineShowCmd (int cmdShow)
{
	if (cmdShow == SW_SHOWDEFAULT)
	{
		//	SW_SHOWDEFAULT instructs Windows to use the show value specified in
		//	the STARTUPINFO passed to CreateProcess.  We want to test against
		//	that final value
		STARTUPINFO startUpInfo;
		::GetStartupInfo (&startUpInfo);
		cmdShow = startUpInfo.wShowWindow;
	}

	if (cmdShow != SW_SHOW && cmdShow != SW_SHOWNORMAL)
		_pl.showCmd = cmdShow;
}
