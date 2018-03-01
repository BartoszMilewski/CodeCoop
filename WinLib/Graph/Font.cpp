// ---------------------------------
// Reliable Software (c) 1998 - 2006
// ---------------------------------

#include <WinLibBase.h>
#include "Font.h"

using namespace Font;

Descriptor::Descriptor (Win::Canvas canvas)
{
	::GetObject (::GetCurrentObject (canvas.ToNative (), OBJ_FONT), sizeof (Descriptor), this);
}

void Descriptor::SetDefault ()
{
    lfHeight         = 0;
    lfWidth          = 0;
    lfEscapement     = 0;
    lfOrientation    = 0;
    lfWeight         = FW_MEDIUM;
    lfItalic         = FALSE;
    lfUnderline      = FALSE;
    lfStrikeOut      = FALSE;
    lfCharSet        = DEFAULT_CHARSET;
    lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lfQuality        = DEFAULT_QUALITY;
    lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
}

Maker::Maker ()
	: _pointSize (0)
{}

Maker::Maker (int pointSize, std::string const & faceName)
	: _pointSize (pointSize)
{
	Init (pointSize, faceName);
}

Maker::Maker (Font::Descriptor const & newFont)
	: Descriptor (newFont),
	  _pointSize (0)
{
	UpdatePointSize ();
}

void Maker::Init (int pointSize, std::string const & faceName)
{
	//Set size for the screen
	_pointSize = pointSize;
	Win::DesktopCanvas desktop;
	ScaleUsing (desktop);
    lfCharSet        = ANSI_CHARSET;
    lfOutPrecision   = OUT_TT_PRECIS;
    lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	SetFaceName (faceName);
}

void Maker::SetFaceName (std::string const & faceName)
{
    Assert (faceName.size () + 1 < LF_FACESIZE);
	strncpy (lfFaceName, faceName.c_str (), faceName.size () + 1);
}

void Maker::SetPointSize (int pointSize)
{
	//Set size for the screen
	_pointSize = pointSize;
	Win::DesktopCanvas desktop;
	ScaleUsing (desktop);
}

void Maker::ScaleUsing (Win::Canvas canvas)
{
	//Set size for the specified canvas
    int logpix = ::GetDeviceCaps (canvas.ToNative (), LOGPIXELSY);
    lfHeight = -MulDiv (_pointSize, logpix, 72);
}

void Maker::SetHeight (int height)
{
	lfHeight = height;
	UpdatePointSize ();
}

Font::AutoHandle Maker::Create ()
{
	Font::Handle font = ::CreateFontIndirect (this);
    if (font.IsNull ())
        throw Win::Exception ("Internal error: Cannot create a font.", GetFaceName ());

	return Font::AutoHandle (font.ToNative ());
}

void Maker::UpdatePointSize ()
{
	// Set size for the screen
	Win::DesktopCanvas canvas;
    int logpix = ::GetDeviceCaps (canvas.ToNative (), LOGPIXELSY);
	int height = lfHeight;
	if (height < 0)
		height = -height;
	_pointSize = ::MulDiv (height, 72, logpix);
}

void Holder::GetAveCharSize(int &aveCharWidth, int &aveCharHeight)
{
    TEXTMETRIC fontMetric;
	Assert (!_canvas.IsNull ());
    ::GetTextMetrics(_canvas.ToNative (), &fontMetric);
    aveCharWidth = fontMetric.tmAveCharWidth;
    // Assert (aveCharWidth > 0);
    aveCharHeight = fontMetric.tmHeight + fontMetric.tmExternalLeading;
    // Revisit: this assertion sometimes fails
    // Assert (aveCharHeight > 0);
}

void Holder::GetBaseUnits (int &baseUnitX, int &baseUnitY)
{
    SIZE size;

    // None of these methods really works!
#if 0
    // method 1
    ::GetTextExtentPoint32(_canvas, "X", 1, &size);

    baseUnitX = size.cx;
    baseUnitY = size.cy;
    // method 2
    ::GetTextExtentPoint32(_canvas, "x", 1, &size);

    baseUnitX = size.cx;
    baseUnitY = size.cy;

    // method 3
    TEXTMETRIC fontMetric;

    ::GetTextMetrics(_canvas, &fontMetric);
    baseUnitX = fontMetric.tmAveCharWidth;
    baseUnitY = fontMetric.tmHeight;
#endif
    // method 4
    char testStr [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    ::GetTextExtentPoint32(_canvas.ToNative (), testStr, strlen (testStr), &size);
    baseUnitX  = size.cx / strlen (testStr);
    baseUnitY = size.cy;

    Assert (baseUnitX > 0);
    Assert (baseUnitY > 0);
}

// Note -- the calculation below works only for fixed pitch fonts
int Holder::GetCharWidthTwips (char c, int pointSize)
{
	TEXTMETRIC txtMetric;
	::GetTextMetrics (_canvas.ToNative (), &txtMetric);
	// 1 point = 20 twips
	int twipsPerPixel = (20 * pointSize) / (txtMetric.tmHeight - txtMetric.tmInternalLeading);
	int width;
	::GetCharWidth32 (_canvas.ToNative (), c, c, &width);
	return width * twipsPerPixel;
}
