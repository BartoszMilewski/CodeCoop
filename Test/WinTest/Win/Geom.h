#if !defined (GEOM_H)
#define GEOM_H
// --------------------------------------
// Reliable Software (c) 2001-2003
// --------------------------------------
#include <Win/Win.h>

// Windows geometry types

namespace Win
{
	class Point: public POINT
	{
	public:
		Point (int xCoord = 0, int yCoord = 0)
		{
			x = xCoord;
			y = yCoord;
		}
		Point (POINT const & pt)
		{
			x = pt.x;
			y = pt.y;
		}
	};

	class Rect: public RECT
	{
	public:
		Rect (RECT const & rect)
		{
			left = rect.left;
			top = rect.top;
			right = rect.right;
			bottom = rect.bottom;
		}
		Rect (int lft = 0, int tp = 0, int rght = 0, int bttm = 0)
		{
			left = lft;
			right = rght;
			top = tp;
			bottom = bttm;
		}
		void ShiftTo (int x, int y)
		{
			::OffsetRect (this, x - left, y - top);
		}
		void ShiftBy (int dx, int dy)
		{
			::OffsetRect (this, dx, dy);
		}
		int Width () const { return right - left; }
		int Height () const { return bottom - top; }
		int Left () const { return left; }
		int Top () const { return top; }
		int Right () const { return right; }
		int Bottom () const { return bottom; }
		void Inflate(int dx, int dy)
		{
			::InflateRect(this, dx, dy);
		}
	};

	class ClientRect: public Win::Rect
	{
	public:
		ClientRect (Win::Dow::Handle win)
		{
			::GetClientRect (win.ToNative (), this);
		}
	};
}
#endif
