#if !defined (TIMER_H)
#define TIMER_H
//
// (c) Reliable Software, 1997, 98, 99, 2000
//

#include <Win/Win.h>

namespace Win
{
	class Timer
	{
	public:
		Timer (int id) 
			: _win (0),
			  _id (id)
		{}

		void Attach (Win::Dow::Handle win)
		{
			_win = win;
		}
		void Set (int milliSec)
		{
			if (!_win.IsNull ())
				_win.SetTimer (_id, milliSec);
		}
		void Kill ()
		{
			if (!_win.IsNull ())
				_win.KillTimer (_id);
		}
		int  GetId () const { return _id; }
		bool operator== (int id) const { return _id == id; }
	private:
		Win::Dow::Handle	_win;
		int					_id;
	};
}

#endif
