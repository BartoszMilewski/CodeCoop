#if !defined (UTILITY_H)
#define UTILITY_H
//----------------------------------
// (c) Reliable Software 2000 - 2005
//----------------------------------

#include <Graph/Canvas.h>
#include <Graph/Brush.h>
#include <Bit.h>

// Wrappers for some Windows data structures

namespace Win
{
	class CreateData
	{
	public:
		void * GetCreationData () const { return _data.lpCreateParams; }
		Win::Instance GetInstance () const { return _data.hInstance; }
		HWND GetParent () const { return _data.hwndParent; }
		int GetHeight () const { return _data.cy; }
		int GetWidth () const { return _data.cx; }
		int GetX () const { return _data.x; }
		int GetY () const { return _data.y; }
		char const * GetWndName () const { return _data.lpszName; }
	private:
		CREATESTRUCT	_data;
	};

	class ActiveAction
	{
	public:
		ActiveAction (int code)
			: _code (code)
		{}

		bool IsActive () const { return _code == WA_ACTIVE; }
		bool IsClickActive () const { return _code == WA_CLICKACTIVE; }
		bool IsInActive () const { return _code == WA_INACTIVE; }

	private:
		int	_code;
	};

	class MouseActiveAction
	{
	public:
		MouseActiveAction ()
			: _code (MA_ACTIVATE)
		{}

		operator LRESULT () const { return _code; }
		void SetActivate () { _code = MA_ACTIVATE; }
		void SetNoActivate () { _code = MA_NOACTIVATE; }
		void SetActivateAndEat () { _code = MA_ACTIVATEANDEAT; }
		void SetNoActivateAndEat () { _code = MA_NOACTIVATEANDEAT; }

	private:
		unsigned int	_code;
	};

	class HitTest
	{
	public:
		HitTest (unsigned int code)
			: _code (code)
		{}

		bool IsBorder () const { return _code == HTBORDER; }
		bool IsBottom () const { return _code == HTBOTTOM; }
		bool IsBottomLeft () const { return _code == HTBOTTOMLEFT; }
		bool IsBottomRight () const { return _code == HTBOTTOMRIGHT; }
		bool IsCaption () const { return _code == HTCAPTION; }
		bool IsClient () const { return _code == HTCLIENT; }
		bool IsClose () const { return _code == HTCLOSE; }
		bool IsNoWhere () const { return _code == HTERROR || _code == HTNOWHERE; }
		bool IsSizeBox () const { return _code == HTGROWBOX || _code == HTSIZE; }
		bool IsHelp () const { return _code == HTHELP; }
		bool IsHScroll () const { return _code == HTHSCROLL; }
		bool IsLeft () const { return _code == HTLEFT; }
		bool IsMenu () const { return _code == HTMENU; }
		bool IsMaximize () const { return _code == HTMAXBUTTON || _code == HTZOOM; }
		bool IsMinimize () const { return _code == HTMINBUTTON || _code == HTREDUCE; }
		bool IsRight () const { return _code == HTRIGHT; }
		bool IsSystemMenu () const { return _code == HTSYSMENU; }
		bool IsTop () const { return _code == HTTOP; }
		bool IsTopLeft () const { return _code == HTTOPLEFT; }
		bool IsTopRight () const { return _code == HTTOPRIGHT; }
		bool IsTransparent () const { return _code == HTTRANSPARENT; }
		bool IsVScroll () const { return _code == HTVSCROLL; }

	private:
		unsigned int	_code;
	};

	class SystemWideFlags
	{
	public:
		SystemWideFlags (WPARAM wParam)
			: _value (wParam)
		{}

		bool IsNonClientMetrics () const { return _value == SPI_SETNONCLIENTMETRICS; }

	private:
		int	_value;
	};

	class KeyState : public BitFieldMask<WPARAM>
	{
	public:
		KeyState (WPARAM wParam)
			: BitFieldMask<WPARAM> (wParam)
		{}
		bool IsCtrl () const { return IsSet (MK_CONTROL); }
		bool IsShift () const { return IsSet (MK_SHIFT); }
		bool IsLButton () const { return IsSet (MK_LBUTTON); }
		bool IsMButton () const { return IsSet (MK_MBUTTON); }
		bool IsRButton () const { return IsSet (MK_RBUTTON); }
	};

#if defined(TESTING)
	//	::GetWindowInfo not available on Windows 95
	class WindowInfo : protected WINDOWINFO
	{
	friend std::ostream& operator<<(std::ostream& os, WindowInfo& wi);

	private:
		Win::Dow::Handle _h;
	public:
		WindowInfo(Win::Dow::Handle win)
		: _h(win)
		{
			cbSize = sizeof(WINDOWINFO);
			BOOL ret = ::GetWindowInfo(_h.ToNative (), this);
			Assert(ret != 0);
		}

		Win::Dow::Handle Window() const
		{
			return _h;
		}
		int VerticalBorderSize() const { return cyWindowBorders; }
		int HorizontalBorderSize() const { return cxWindowBorders; }
	};
#endif

	class ClassInfo : private WNDCLASSEX
	{
	public:
		ClassInfo(Win::Instance hInstance, char const *className)
		{
			cbSize = sizeof(WNDCLASSEX);
			BOOL ret = ::GetClassInfoEx(hInstance, className, this);
			Assert(ret != 0);
		}

		Brush::Handle GetBgBrush() const
		{ return hbrBackground; }
	};

#if defined(TESTING)
	inline std::ostream& operator<<(std::ostream& os, WindowInfo& wi)
	{
		os << "Window Information for " << std::hex << wi.Window().ToNative () << std::endl;

		if (wi.dwStyle & WS_CHILD)
		{
			HWND hwndParent = ::GetParent(wi.Window().ToNative ());
			os << "  Parent is: " << std::hex << hwndParent << std::endl;
		}

		os << std::dec;
		os << "  Window Rect: " << wi.rcWindow << std::endl;
		os << "  Client Rect: " << wi.rcClient << std::endl;
		os << std::hex;
		os << "  Style: " << wi.dwStyle << std::endl;
		os << "  Extended Style: " << wi.dwExStyle << std::endl;
		os << std::dec;
		os << "  cx cy borders: " << wi.cxWindowBorders << ", " << wi.cyWindowBorders;

		return os;
	}
#endif
}

#endif
