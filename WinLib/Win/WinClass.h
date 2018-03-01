#if !defined (WINCLASS_H)
#define WINCLASS_H
//
// (c) Reliable Software, 1997-2002
//
#include <Win/GdiHandles.h>
#include <Win/Procedure.h>
#include <Sys/WinString.h>

namespace Win
{
	namespace Class
	{
		struct Style
		{
		public:
			enum Bits
			{
				DblClicks = CS_DBLCLKS,
				HRedraw = CS_HREDRAW,
				VRedraw = CS_VREDRAW,
				// Redraw the whole window every time size changes
				RedrawOnSize = CS_HREDRAW | CS_VREDRAW
			};
		public:
			Style () : _style (0) {}
			void OrIn (Style::Bits bits)
			{
				_style |= bits;
			}
		private:
			DWORD _style;
		};

		inline Style& operator<<(Style & style, Style::Bits bits)
		{
			style.OrIn (bits);
			return style;
		}

		class Maker : public WNDCLASSEX
		{
		public:
			Maker (int idClassName, Win::Instance hInst, WNDPROC WndProc = Win::Procedure);
			Maker (char const * className, Win::Instance hInst, WNDPROC WndProc = Win::Procedure);
			Win::Class::Style & Style () { return reinterpret_cast<Win::Class::Style &> (style); }
			void SetBgSysColor (int sysColor)
			{
				hbrBackground = (HBRUSH) (sysColor + 1);
			}

			void SetBgBrush (Brush::Handle hbr)
			{
				hbrBackground = hbr.ToNative ();
			}

			HWND GetRunningWindow ();

			void SetSysCursor (char const * id) 
			{ 
				hCursor = ::LoadCursor (0, id); 
			}
			void SetResCursor (int id) 
			{ 
				HCURSOR hCur = ::LoadCursor (hInstance, MAKEINTRESOURCE (id));
				hCursor = hCur; 
			}
			void SetResIcons (int resId);
			void AddExtraLong () { cbWndExtra += 4; }
			void Register ();
			bool IsRegistered () const;
		protected:
			Icon::Handle	_stdIcon;
			Icon::Handle	_smallIcon;
		private:
			void SetDefaults (WNDPROC wndProc, Win::Instance hInst);

			ResString	_classString;
		};

		class TopMaker: public Maker
		{
		public:
			TopMaker (int idClassName, Win::Instance hInst, int resId = 0, WNDPROC WndProc = Win::Procedure);
			TopMaker (char const * className, Win::Instance hInst, int resId = 0, WNDPROC wndProc = Win::Procedure);
		};
	}
}

#endif
