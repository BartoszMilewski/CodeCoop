#if !defined (REGION_H)
#define REGION_H
// Copyright Reliable Software 2000
#include <Win/GdiHandles.h>
#include <Win/Geom.h>

namespace Win
{
	class Point;
	class Canvas;
}

namespace Region
{
	class Handle : public Win::Handle<HRGN>
	{
	public:
		Handle (HRGN h = 0) 
			: Win::Handle<HRGN> (h)
		{}
		bool IsInside (int x, int y)
		{
			return ::PtInRegion (ToNative (), x, y) != FALSE;
		}
		void Fill (Win::Canvas canvas, Brush::Handle brush);
	};

	class AutoHandle: public Win::AutoHandle<Handle, Gdi::Disposal<Region::Handle> >
	{
	public:
		AutoHandle (HRGN h = NullValue ())
			: Win::AutoHandle<Handle, Gdi::Disposal<Region::Handle> > (h)
		{}
	};

	class Elliptic: public AutoHandle
	{
	public:
		Elliptic (int xLeft, int yUp, int xRight, int yDn);
	};

	class Rectangular: public AutoHandle
	{
	public:
		Rectangular (int xLeft, int yUp, int xRight, int yDn);
		Rectangular (Win::Rect rect);
	};

	class Rounded: public AutoHandle
	{
	public:
		Rounded (int xLeft, int yUp, int xRight, int yDn, 
				 int ellipseWidth, int ellipseHeight);
	};

	class Polygonal: public AutoHandle
	{
	public:
		Polygonal (std::vector<Win::Point> const & points, bool isFilled = true);
	};
}

#endif
