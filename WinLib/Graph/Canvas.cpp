//------------------------------------
//  (c) Reliable Software, 1997 - 2006
//------------------------------------

#include <WinLibBase.h>
#include <Graph/Canvas.h>
#include <Graph/Pen.h>
#include <Graph/Bitmap.h>
#include <Graph/Brush.h>
#include <Win/Geom.h>

using namespace Win;

Gdi::Handle Canvas::SelectObject (Gdi::Handle newObject)
{
	Gdi::Handle oldObject (::SelectObject (H (), newObject.ToNative ()));
	return oldObject;
}

void Canvas::Frame (Win::Rect const & rect, HPEN pen)
{
	Pen::Holder penHolder (H (), pen);
    ::MoveToEx (H (), rect.left, rect.bottom, 0);
    ::LineTo (H (), rect.left, rect.top);
    ::LineTo (H (), rect.right, rect.top);
    ::LineTo (H (), rect.right, rect.bottom);
    ::LineTo (H (), rect.left, rect.bottom);
}

void Canvas::Frame (Win::Rect const & rect, HBRUSH brush)
{
	::FrameRect (H (), &rect, brush);
}

void Canvas::FillRect (Win::Rect const & rect, HBRUSH brush)
{
    ::FillRect (H (), &rect, brush);
}

void Canvas::GetCharSize (int& cxChar, int& cyChar)
{
    TEXTMETRIC tm;
    ::GetTextMetrics (H (), &tm);
    cxChar = tm.tmAveCharWidth;
    cyChar = tm.tmHeight + tm.tmExternalLeading;
}

void Canvas::GetTextSize (char const * text, int& cxText, int& cyText, size_t lenString)
{
	if (lenString == 0)
		lenString = strlen (text);
    SIZE textSize;
    ::GetTextExtentPoint32 (H (), text, lenString, &textSize);
    cxText = textSize.cx;
    cyText = textSize.cy;
}

void Canvas::PaintBitmap (Bitmap::Handle bitmap, 
						  int width, 
						  int height, 
						  int xDest, 
						  int yDest, 
						  int xSrc, 
						  int ySrc,
					      DWORD rop)
{
	MemCanvas memCanvas (H ());
	Bitmap::Holder holder (memCanvas, bitmap);
	::BitBlt (H (), 
			xDest, 
			yDest, 
			width, 
			height, 
			memCanvas.ToNative (), 
			xSrc, 
			ySrc, 
			rop);
}

void Canvas::StretchBitmap (Bitmap::Handle bitmap, 
						    int srcWidth, 
						    int srcHeight, 
						    int dstWidth, 
						    int dstHeight, 
						    int xDest, 
						    int yDest, 
						    int xSrc, 
						    int ySrc,
						    DWORD rop)
{
	MemCanvas memCanvas (H ());
	Bitmap::Holder holder (memCanvas, bitmap);
	::StretchBlt (H (), 
				xDest, 
				yDest, 
				dstWidth, 
				dstHeight, 
				memCanvas.ToNative (), 
				xSrc, 
				ySrc,
				srcWidth,
				srcHeight,
				rop);
}

void PaintCanvas::EraseBackground (Win::Color backgroundColor)
{
	Brush::AutoHandle brush (backgroundColor);
	::FillRect (H (), &_paint.rcPaint, brush.ToNative ());
}
