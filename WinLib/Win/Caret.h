#if !defined (WINCARET_H)
#define WINCARET_H
//  (c) Reliable Software, 2001
#include <Win/Geom.h>

namespace Win
{
	class Caret
	{
	public:
		Caret (Win::Dow::Handle win)
			: _win (win)
		{}
		void Create (int width, int height)
		{
			::CreateCaret (_win.ToNative (), 0, width, height);
			::ShowCaret (_win.ToNative ());
		}
		void Kill ()
		{
			::DestroyCaret ();
		}
		void SetPosition (Win::Point const & p)
		{
			::SetCaretPos (p.x, p.y);
		}
		void GetPosition (Win::Point & p) const
		{
			::GetCaretPos (&p);
		}
	private:
		Win::Dow::Handle	_win;
	};
}

#endif
