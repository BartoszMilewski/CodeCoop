#if !defined (WINCLASS_H)
#define WINCLASS_H
//
// (c) Reliable Software, 1997-2002
//
#include <Win/Procedure.h>
#include <Win/Instance.h>

namespace Win
{
	namespace Class
	{
		class Style
		{
		public:
			Style& operator<<(void (Style::*method)())
			{
				(this->*method)();
				return *this;
			}
			void DblClicks ()
			{
				_style |= CS_DBLCLKS;
			}
			void RedrawOnSize ()
			{
				// Redraw the whole window every time size changes
				_style |= (CS_HREDRAW | CS_VREDRAW);
			}
		private:
			DWORD _style;
		};

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
			void AddExtraLong () { cbWndExtra += 4; }
			void Register ();
			bool IsRegistered () const;
		private:
			void SetDefaults (WNDPROC wndProc, Win::Instance hInst);
		};

		class TopMaker: public Maker
		{
		public:
			TopMaker (char const * className, Win::Instance hInst, int resId, WNDPROC wndProc = Win::Procedure);
		};
	}
}

#endif
