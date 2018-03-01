// Copyright Reliable Software 2000
#include <WinLibBase.h>
#include "Region.h"
#include <Graph/Canvas.h>
#include <Graph/Brush.h>

namespace Region
{
	void Handle::Fill (Win::Canvas canvas, Brush::Handle brush)
	{
		::FillRgn (canvas.ToNative (), ToNative (), brush.ToNative ());
	}
	Elliptic::Elliptic (int xLeft, int yUp, int xRight, int yDn)
		: AutoHandle (::CreateEllipticRgn (xLeft, yUp, xRight, yDn))
	{
		if (IsNull ())
			throw Win::Exception ("Cannot create region");
	}
	Rectangular::Rectangular (int xLeft, int yUp, int xRight, int yDn)
		: AutoHandle (::CreateRectRgn (xLeft, yUp, xRight, yDn))
	{
		if (IsNull ())
			throw Win::Exception ("Cannot create region");
	}
	Rectangular::Rectangular (Win::Rect rect)
		: AutoHandle (::CreateRectRgn (rect.Left (), rect.Top (), rect.Right (), rect.Bottom ()))
	{
		if (IsNull ())
			throw Win::Exception ("Cannot create region");
	}
	Rounded::Rounded (int xLeft, int yUp, int xRight, int yDn, int ellipseWidth, int ellipseHeight)
		: AutoHandle (::CreateRoundRectRgn (xLeft, yUp, xRight, yDn, 
											ellipseWidth, ellipseHeight))
	{
		if (IsNull ())
			throw Win::Exception ("Cannot create region");
	}

	Polygonal::Polygonal (std::vector<Win::Point> const & points, bool isFilled)
		: AutoHandle (::CreatePolygonRgn (&points [0], 
										points.size (), 
										isFilled? WINDING: ALTERNATE))
	{
		if (IsNull ())
			throw Win::Exception ("Cannot create region");
	}
}