//
// (c) Reliable Software, 1997-2000
//
#include <WinLibBase.h>
#include "WinClass.h"

#include <Win/Instance.h>
#include <Graph/Icon.h>

namespace Win
{
	namespace Class
	{
		Maker::Maker (int idClassName, Win::Instance hInst, WNDPROC wndProc)
			: _classString (hInst, idClassName)
		{
			lpszClassName = _classString;
			SetDefaults (wndProc, hInst);
		}

		Maker::Maker (char const * className, Win::Instance hInst, WNDPROC wndProc)
		{
			lpszClassName = className;
			SetDefaults (wndProc, hInst);
		}

		void Maker::SetDefaults (WNDPROC wndProc, Win::Instance hInst)
		{
			// Provide reasonable default values
			cbSize = sizeof (WNDCLASSEX);
			style = 0;
			lpfnWndProc = wndProc;
			hInstance = hInst;
			hIcon = 0;
			hIconSm = 0;
			lpszMenuName = 0;
			cbClsExtra = 0;
			cbWndExtra = 0;
            hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			hCursor = ::LoadCursor (0, IDC_ARROW);
		}

		void Maker::SetResIcons (int resId)
		{
			Icon::StdMaker stdMk;
			_stdIcon = stdMk.Load (hInstance, resId);
			hIcon = _stdIcon.ToNative ();

			Icon::SmallMaker smallMk;
			_smallIcon = smallMk.Load (hInstance, resId);
			hIconSm = _smallIcon.ToNative ();
		}

		HWND Maker::GetRunningWindow ()
		{
			HWND hwnd = ::FindWindow (lpszClassName, 0);
			if (::IsWindow (hwnd))
			{
				HWND hwndPopup = ::GetLastActivePopup (hwnd);
				if (::IsWindow (hwndPopup))
					hwnd = hwndPopup;
			}
			else 
				hwnd = 0;

			return hwnd;
		}

		void Maker::Register ()
		{
			if (::RegisterClassEx (this) == 0)
			{
				int err = Win::GetError ();
				if (err != ERROR_CLASS_ALREADY_EXISTS)
					throw Win::Exception ("Internal error: RegisterClassEx failed.", lpszClassName, err);
			}
			Win::ClearError (); // Windows bug: sets error even when successful
		}

		bool Maker::IsRegistered () const
		{
			WNDCLASSEX wndClass;
			return ::GetClassInfoEx (hInstance, lpszClassName, &wndClass) != 0;
		}

		TopMaker::TopMaker (int idClassName, Win::Instance hInst, int resId, WNDPROC wndProc)
			: Maker (idClassName, hInst, wndProc)
		{
			style = 0;
			if (resId != 0)
			{
				SetResIcons (resId);
				lpszMenuName = MAKEINTRESOURCE (resId);
			}
		}

		TopMaker::TopMaker (char const * className, Win::Instance hInst, int resId, WNDPROC wndProc)
			: Maker (className, hInst, wndProc)
		{
			style = 0;
			if (resId != 0)
			{
				SetResIcons (resId);
				lpszMenuName = MAKEINTRESOURCE (resId);
			}
		}
	}
};
