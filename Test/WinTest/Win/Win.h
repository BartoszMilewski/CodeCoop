#if !defined (WIN_DOW_H)
#define WIN_DOW_H
// (c) Reliable Software (c) 1998-03
#include <Win/Instance.h>
#include <Win/WinEx.h>
#include <string>
#include <assert.h>

namespace Win
{
	class Message;
	class RegisteredMessage;
	class Placement;
	class Controller;
	class SubController;
	class Point;
	class Rect;

	// To set various styles, use operator << , for instance
	// style << AddClientEdge << AddVScrollBar << AddHScrollBar;
	class Style
	{
	public:
		Style (unsigned long style = 0, unsigned long exStyle = 0)
			: _style (style), _exStyle (exStyle) 
		{}
		Style& operator<< (void (Style::*method)())
		{
			(this->*method)();
			return *this;
		}
		unsigned long GetStyleBits () { return _style; }
		unsigned long GetExStyleBits () { return _exStyle; }
		void SetMaximized () { _style |= WS_MAXIMIZE; }
		void AddSysMenu () { _style |= WS_SYSMENU; }
		void AddVScrollBar () { _style |= WS_VSCROLL; }
		void AddHScrollBar () { _style |= WS_HSCROLL; }
		void AddDlgBorder () { _style |= WS_DLGFRAME; }
		void AddBorder () { _style |= WS_BORDER; }
		void AddTitleBar () { _style |= WS_CAPTION; }
		void ClipChildren() { _style |= WS_CLIPCHILDREN; }
		void ClipSiblings() { _style |= WS_CLIPSIBLINGS; }
		void AddClientEdge() { _exStyle |= WS_EX_CLIENTEDGE; }
		void Visible () { _style |= WS_VISIBLE; }
		void Child () { _style |= WS_CHILD; }
		void Popup () { _style |= WS_POPUP; }
		void OverlappedWindow () { _style |= WS_OVERLAPPEDWINDOW; }
		// Turn off the "visible" bit for those makers that make visible windows by default
		void SetInvisible () { _style &= ~WS_VISIBLE; }
		void DontClipChildren () { _style &= ~WS_CLIPCHILDREN; }
	protected:
		DWORD _style;
		DWORD _exStyle;
	};
	
	//-----------------
	// Global functions
	//-----------------
	void Quit (int errorCode = 0);
	inline void Sleep (unsigned milliSec) { ::Sleep (milliSec); }

	// Don't use directly, Access as Win::Dow::Handle
	class window_handle: public Handle<HWND>
	{
	public:
		window_handle (HWND h = NullValue ())
			: Handle<HWND> (h) 
		{}
		void Reset (window_handle h = window_handle ())
		{
			Handle<HWND>::Reset (h.ToNative ());
		}
		// Get/Set Window Long
		template <class T>
		inline T GetLong (int which = GWL_USERDATA)
		{
			return reinterpret_cast<T> (::GetWindowLong (_h, which));
		}
		template<>	// specialization for long
		inline long GetLong (int which)
		{
			return ::GetWindowLong (_h, which);
		}

		template <class T>
		inline T SetLong (T value, int which = GWL_USERDATA)
		{
			return reinterpret_cast<T> (::SetWindowLong (_h, which, reinterpret_cast<long> (value)));
		}

		template<>	// specialization for long
		inline long SetLong<long> (long value, int which)
		{
			return ::SetWindowLong (_h, which, value);
		}
		template<>	// specialization for unsigned long
		inline unsigned long SetLong<unsigned long> (unsigned long value, int which)
		{
			return static_cast<unsigned long> (::SetWindowLong (_h, which, static_cast<long> (value)));
		}
		Win::Style GetStyle ()
		{
			return Win::Style ( ::GetWindowLong (_h, GWL_STYLE),
								::GetWindowLong (_h, GWL_EXSTYLE));
		}
		Win::Style SetStyle (Win::Style style)
		{
			return Win::Style (SetLong (style.GetStyleBits (), GWL_STYLE),
							   SetLong (style.GetExStyleBits (), GWL_EXSTYLE));
		}
		// Used to work around a bug in tracking context menu
		void ForceTaskSwitch ()
		{
			::PostMessage (_h, WM_NULL, 0, 0);
		}
		// Messages
		LRESULT SendMsg (UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const
		{
			assert (_h != 0);
			return ::SendMessage (_h, msg, wparam, lparam);
		}
		void SendMsg (Message & msg) const;
		bool PostMsg (UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const
		{
			assert (_h != 0);
			return ::PostMessage (_h, msg, wparam, lparam) != FALSE;
		}
		bool PostMsg (Message const & msg) const;
		void SendInterprocessPackage (RegisteredMessage & msg) const;
		Win::Instance GetInstance () const
		{ 
			return reinterpret_cast<HINSTANCE> (::GetWindowLong (_h, GWL_HINSTANCE));		
		}
		//	return the window that defines the drawing origin
		window_handle GetParent () const
		{
			//	Windows ::GetParent returns the owner window for pop-up
			//	windows, so we only use it if this is a child window
			if (::GetWindowLong (_h, GWL_STYLE) & WS_CHILD)
			{
				return ::GetParent(_h);
			}

			//	for top level windows the drawing origin is defined by the desktop
			return ::GetDesktopWindow();
		}
		window_handle GetOwner () const
		{
			//	child windows are owned by their parents
			if (::GetWindowLong (_h, GWL_STYLE) & WS_CHILD)
			{
				return ::GetParent(_h);
			}

			//	::GetWindow will only return the owner for non-child windows
			return ::GetWindow(_h, GW_OWNER);
		}
		Win::Controller * GetController ()
		{
			return reinterpret_cast<Win::Controller *>(::GetWindowLong (_h, GWL_USERDATA));
		}
		void SetParent (Handle<HWND> hwndParent)
		{
			::SetParent (_h, hwndParent.ToNative ());
		}
		int GetId () const // child windows and controls have ids
		{
			assert (!IsNull ());
			return ::GetWindowLong (_h, GWL_ID);
		}
		// Destroying
		void Destroy ()
		{
			::DestroyWindow (_h);
		}
		// Change window class properties
		void SetClassHRedraw (bool yes)
		{
			// redraw whole window on horizontal resize
			DWORD style = ::GetClassLong (_h, GCL_STYLE);
			if (yes)
				style |= CS_HREDRAW;
			else
				style &= ~CS_HREDRAW;
			::SetClassLong (_h, GCL_STYLE, style);
		}
		 
		// Coordinates
		void ClientToScreen (Win::Point & pt) const;
		// Messages
		long SendMessageTo (int idChild, UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const
		{
			return static_cast<long> (::SendDlgItemMessage (_h, idChild, msg, wparam, lparam));
		}
		// Focus
		void SetFocus ()
		{ 
			::SetFocus (_h); 
		}
		bool HasFocus () const
		{
			return ::GetFocus () == _h;
		}
		// Timer
		void SetTimer (int idTimer, int milliSec)
		{
			if (::SetTimer (_h, idTimer, milliSec, 0) == 0)
				throw Win::Exception ("Internal error: Cannot set timer.");
		}
		void KillTimer (int idTimer)
		{
			::KillTimer (_h, idTimer);
		}
		// Mouse capture
		void CaptureMouse ()
		{
			::SetCapture (_h);
		}
		static void ReleaseMouse ()
		{
			::ReleaseCapture ();
		}
		bool HasCapture () const
		{
			return ::GetCapture () == _h;
		}
		// Text/Caption/Class name
		void SetText (std::string const & text)
		{
			SetText (text.c_str ());
		}
		void SetText (char const * text) 
		{ 
			::SetWindowText (_h, text); 
		}
		int GetText (char * buf, int len) const// len includes null
		{ 
			return ::GetWindowText (_h, buf, len); 
		}
		int GetTextLength () const
		{
			return ::GetWindowTextLength(_h);
		}
		int GetClassName (char * buf, int len) const
		{
			// Revisit: there is better API RealGetWindowClass, but it is supported
			// only in Win98, NT 4.0 with SP3 and Win2000
			return ::GetClassName (_h, buf, len);
		}
		// Visibility
		bool Exists () const
		{
			// An application should not use Exists for a window that it did not create
			// because the window could be destroyed after this function was called.
			// Further, because window handles are recycled the handle could even point
			// to a different window.
			return ::IsWindow (_h) != 0;
		}
		void Enable ()
		{
			::EnableWindow (_h, TRUE);
		}

		void Disable ()
		{
			::EnableWindow (_h, FALSE);
		}

		void Show (int cmdShow = SW_SHOW) 
		{ 
			::ShowWindow (_h, cmdShow); 
		}

		void ShowMaximized () { Show (SW_SHOWMAXIMIZED); }
		void ShowMinimized () { Show (SW_SHOWMINIMIZED); }

		void Restore ()
		{ 
			::ShowWindow (_h, SW_RESTORE); 
		}

		void Hide () 
		{ 
			::ShowWindow (_h, SW_HIDE); 
		}
		void Update () 
		{ 
			::UpdateWindow (_h); 
		}
		void SetForeground ()
		{
			::SetForegroundWindow (_h);
		}
		void BringToTop ()
		{
			::BringWindowToTop (_h);
		}
		void Display (int cmdShow = SW_SHOW)
		{
			Show (cmdShow);
			Update ();
		}
		// Moving
		void Move (int x, int y, int width, int height)
		{
			::MoveWindow (_h, x, y, width, height, TRUE);
		}
		void MoveDelayPaint (int x, int y, int width, int height)
		{
			::MoveWindow (_h, x, y, width, height, FALSE);
		}
		// Repainting
		void SetRedraw (bool value)
		{
			::SendMessage (_h, WM_SETREDRAW, value? TRUE: FALSE, 0);
		}
		void Invalidate ()
		{
			::InvalidateRect (_h, 0, TRUE);
		}
		void Invalidate (Win::Rect const & rect);
		void ForceRepaint ()
		{
			Invalidate ();
			Update ();
		}
		// Scrolling
		void Scroll (int xAmount, int yAmount)
		{
			::ScrollWindow (_h, xAmount, yAmount, 0, 0);
		}
		void Scroll (Win::Rect & rect, int xAmount, int yAmount);
		// Rectangles
		void GetWindowRect (Win::Rect & rect) const;
		void GetClientRect (Win::Rect & rect) const;

		//	Subclassing
		void SubClass (SubController * ctrl);
		void UnSubClass ();
	};

	class Dow
	{
	public:
		typedef window_handle Handle;
		typedef AutoHandle<Win::Dow::Handle> Owner;
	};

	class Placement
	{
	public:
		Placement ();
		Placement (Dow::Handle win);
		void Init (Dow::Handle win);

		int  GetFlags () const;
		bool IsMaximized () const;
		bool IsMinimized () const;
		bool IsHidden () const;
        void GetRect (Win::Rect & rect) const;
		void GetMaxCorner (Win::Point & pt) const;
		void GetMinCorner (Win::Point & pt) const;

		void CombineShowCmd (int cmdShow);
		void SetFlags (int flags);
		void SetMaximized ();
		void SetMinimized ();
		void SetHidden ();
		void SetNormal ();
		void SetRect (Win::Rect & rect);
		void SetMaxCorner (Win::Point & point);
		void SetMinCorner (Win::Point & point);

		WINDOWPLACEMENT const * Get () const { return &_pl; }

	private:
		WINDOWPLACEMENT _pl;
	};

	// For temporarily switching window style
	// Modify this style as needed and then call Switch
	class StyleHolder: public Win::Style
	{
	public:
		StyleHolder (Win::Dow::Handle win)
			: Win::Style (win.GetStyle ()), _win (win)  
		{
			// remember the old style for the destructor
			_oldStyle = Win::Style (GetStyleBits (), GetExStyleBits ());
		}
		~StyleHolder ()
		{
			_win.SetStyle (_oldStyle);
		}
		void Switch () { _win.SetStyle (*this); }
	private:
		Win::Dow::Handle _win;
		Win::Style _oldStyle;
	};

	// Temporarily delay repainting of window (and its children)
	// Note: it causes annoying desktop blinking
	class PaintLock
	{
	public:
		PaintLock (Win::Dow::Handle h)
		{
			if (!h.IsNull ())
			{
				BOOL result = ::LockWindowUpdate (h.ToNative ());
				// assert that we are not nesting update locks
				assert (result != 0);
			}
		}
		~PaintLock ()
		{
			::LockWindowUpdate (0);
		}
	};

	class RedrawLock
	{
	public:
		RedrawLock (Win::Dow::Handle h)
			: _h (h)
		{
			_h.SetRedraw (false);
		}
		~RedrawLock ()
		{
			_h.SetRedraw (true);
			_h.Invalidate ();
		}
	private:
		Win::Dow::Handle _h;
	};
}

#endif
