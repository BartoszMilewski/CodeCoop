#if !defined (GEOM_H)
#define GEOM_H
// ---------------------------------
// Reliable Software (c) 2001 - 2005
// ---------------------------------

#if !defined NDEBUG
#include <ostream>
#endif

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

	class MsgPosPoint: public Point
	{
	public:
		MsgPosPoint ();
	};

	class CursorPosPoint: public Point
	{
	public:
		CursorPosPoint ();
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
			right =  (rght < lft ? lft : rght);
			top = tp;
			bottom = (bttm < top ? top : bttm);
		}
		void ShiftTo (int x, int y)
		{
			::OffsetRect (this, x - left, y - top);
		}
		void ShiftBy (int dx, int dy)
		{
			::OffsetRect (this, dx, dy);
		}
		bool IsEmpty () const { return ::IsRectEmpty (this) != 0; }
		bool IsInside (Win::Point const & pt) const { return ::PtInRect (this, pt) != 0; }
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
		void Intersect (Rect const & otherRect, Rect & intersectionRect)
		{
			::IntersectRect (&intersectionRect, this, &otherRect);
		}
		// pick maximum with and height
		void Maximize (int width, int height);
	};

	class ClientRect: public Win::Rect
	{
	public:
		ClientRect (Win::Dow::Handle win)
		{
			::GetClientRect (win.ToNative (), this);
		}
	};

	std::ostream& operator<<(std::ostream& os, Win::Rect const& rc);
	std::ostream& operator<<(std::ostream& os, Win::Point const& pt);
}
#endif
