//--------------------------------
// (c) Reliable Software, 1997-03
//--------------------------------
#include <WinLibBase.h>
#include "WinMaker.h"

#include <Win/Controller.h>

namespace Win
{
	Maker::Maker (int idClassName, Win::Instance hInst)
	  : _classString (hInst, idClassName),
		_className (_classString), // pointer to registered class name
		_windowName (0),    // pointer to window name
		_x (CW_USEDEFAULT), // horizontal position of window
		_y (0),             // vertical position of window
		_width (CW_USEDEFAULT), // window width  
		_height (0),        // window height
		_hWndParent (0),    // handle to parent or owner window
		_hMenu (0),         // handle to menu, or child-window identifier
		_hInstance (hInst), // handle to application instance
		_data (0)           // pointer to window-creation data
	{
	}

	Maker::Maker (char const * className, Win::Instance hInst)
	  : _className (className), // pointer to registered class name
		_windowName (0),    // pointer to window name
		_x (CW_USEDEFAULT), // horizontal position of window
		_y (0),             // vertical position of window
		_width (CW_USEDEFAULT), // window width  
		_height (0),        // window height
		_hWndParent (0),    // handle to parent or owner window
		_hMenu (0),         // handle to menu, or child-window identifier
		_hInstance (hInst), // handle to application instance
		_data (0)           // pointer to window-creation data
	{
	}

	void Maker::SetPosition (int x, int y, int width, int height)
	{
		_x = x;
		_y = y;
		_width = width;
		_height = height;
	}

	Win::Dow::Handle Maker::Create (Controller * controller, char const * title)
	{
		if (controller->MustDestroy ())
		{
			Win::ClearError ();
			throw Win::Exception ("Win::Maker::Create cannot accept Win::TopController");
		}
		HWND h = ::CreateWindowEx (
			_style.GetExStyleBits (),
			_className,
			title? title: _windowName,
			_style.GetStyleBits (),
			_x,
			_y,
			_width,
			_height,
			_hWndParent.ToNative (),
			_hMenu,
			_hInstance,
			controller);

		if (h == 0)
			throw Win::Exception ("Internal error: Window Creation Failed.", title);
		return h;
	}

	Win::Dow::Handle Maker::Create ()
	{
		HWND h = ::CreateWindowEx (
			_style.GetExStyleBits (),
			_className,
			_windowName,
			_style.GetStyleBits (),
			_x,
			_y,
			_width,
			_height,
			_hWndParent.ToNative (),
			_hMenu,
			_hInstance,
			_data);

		if (h == 0)
			throw Win::Exception ("Internal error: Window Creation Failed.");
		return h;
	}

	Win::Dow::Handle Maker::Create (std::unique_ptr<Win::Controller> controller, char const * title)
	{
		Assert (controller->MustDestroy ());
		HWND h = ::CreateWindowEx (
			_style.GetExStyleBits (),
			_className,
			title? title: _windowName,
			_style.GetStyleBits (),
			_x,
			_y,
			_width,
			_height,
			_hWndParent.ToNative (),
			_hMenu,
			_hInstance,
			controller.release ());

		if (h == 0)
			throw Win::Exception ("Internal error: Top Window Creation Failed.", title);
		return h;
	}

	TopMaker::TopMaker (char const * caption, int idClassName, Win::Instance hInst)
		: Maker (idClassName, hInst)
	{
		_style << Win::Style::OverlappedWindow;
		_windowName = caption;
	}

	TopMaker::TopMaker (char const * caption, char const * className, Win::Instance hInst)
		: Maker (className, hInst)
	{
		_style << Win::Style::OverlappedWindow;
		_windowName = caption;
	}

	ChildMaker::ChildMaker (int idClassName, Win::Dow::Handle hwndParent, int childId)
		: Maker (idClassName, hwndParent.GetInstance ())
	{
		_style << Win::Style::Child;
		_hWndParent = hwndParent;
		_hMenu = (HMENU) childId;
	}

	ChildMaker::ChildMaker (char const * className, Win::Dow::Handle hwndParent, int childId)
		: Maker (className, hwndParent.GetInstance ())
	{
		_style << Win::Style::Child;
		_hWndParent = hwndParent;
		_hMenu = (HMENU) childId;
	}

	PopupMaker::PopupMaker (int idClassName, Win::Instance hInst)
		: Maker (idClassName, hInst)
	{
		_style << Win::Style::Popup << Win::Style::Visible;
	}

	PopupMaker::PopupMaker (char const * className, Win::Instance hInst)
		: Maker (className, hInst)
	{
		_style << Win::Style::Popup << Win::Style::Visible;
	}
};
