//--------------------------------
//  (c) Reliable Software, 2001-04
//--------------------------------
#include <WinLibBase.h>
#include "Geom.h"

Win::MsgPosPoint::MsgPosPoint ()
{
	DWORD pos = ::GetMessagePos ();
	POINTS points = MAKEPOINTS (pos);
	x = points.x;
	y = points.y;
}

Win::CursorPosPoint::CursorPosPoint ()
{
	::GetCursorPos (this);
}

void Win::Rect::Maximize (int width, int height)
{
	if (width > Width ())
		right = left + width;
	if (height > Height ())
		bottom = top + height;
}

#if !defined NDEBUG

std::ostream& Win::operator<<(std::ostream& os, Win::Rect const& rc)
{
	os << rc.left << ", " << rc.top << ", "
		<< rc.right << ", " << rc.bottom
		<< " (" << rc.right - rc.left << ", " << rc.bottom - rc.top << ")";
	return os;
}

std::ostream& Win::operator<<(std::ostream& os, Win::Point const& pt)
{
	os << "(" << pt.x << ", " << pt.y << ")";
	return os;
}
#endif