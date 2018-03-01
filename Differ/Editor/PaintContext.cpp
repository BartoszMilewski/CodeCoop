//--------------------------------
// (c) Reliable Software 1997-2005
//--------------------------------
#include "precompiled.h"
#include "PaintContext.h"
#include "Lines.h"
#include <Dbg/Assert.h>
#include <Graph/Brush.h>
#include <Graph/Pen.h>
#include <Graph/CanvTools.h>

FontBox::FontBox (Win::Dow::Handle win, bool useTabs, int tabSizeChar)
	: _useTabs(useTabs), _tabSizeChar (tabSizeChar)
{
	Font::Maker maker (8, "Courier New");
	_font = maker.Create ();
	Win::UpdateCanvas canvas (win);
	Font::Holder fontHolder (canvas, _font);
	canvas.GetCharSize (_xChar, _yChar);
	_tabSizePix = _tabSizeChar * _xChar;
}

void FontBox::ChangeFont (Win::Dow::Handle win, unsigned tabSizeChar, Font::Maker * newFont)
{
	if (tabSizeChar != 0)
		_tabSizeChar = tabSizeChar;
	if (newFont)
		_font = newFont->Create ();
	Win::UpdateCanvas canvas (win);
	Font::Holder fontHolder (canvas, _font);
	canvas.GetCharSize (_xChar, _yChar);
	_tabSizePix = _tabSizeChar * _xChar;
}


// Printing "RSTUVWXYZ" from the following line:
// Document line:
//  |------ start -------><- len ->
//  |....ABCDEFGHIJKLMNOPQRSTUVWXYZ
// In a window that is scrolled horizontally (xOrg negative!)
//               |-----------------------|
//  |<--- xOrg --|JKLMNOPQ               |
//               |-- x -->RSTUVWXYZ      |
//
// Tab expansion in the displayed string is influenced not only by its position
// in the window, but also by the window's scroll offset

int PaintContext::SimpleTextOut (int xOrg, 
								 int x, 
								 int y, 
								 EditParagraph const * para, 
								 int start, 
								 int len)
{
	Assert (x >= 0);
	Assert (start >= 0);
	Assert (len > 0);
	if (start + len > para->Len ())
		len = para->Len () - start;
	if (len <= 0)
		return 0;
	return TabbedText ( x, // start painting at this coordinate
						y, 
						para->Buf () + start, // buffer with characters
						len, // how many characters
						_fontBox.GetTabSizePix (), // tab size in pixels
						xOrg); // starting coordinate for tab expansion
}

int PaintContext::HighTextOut (int xOrg, 
							   int x, 
							   int y, 
							   EditParagraph const * para, 
							   int start, 
							   int len)
{
	EditStyle style (EditStyle::chngNone, EditStyle::actNone);
	Font::BkgHolder bkgCol (*this, _fontBox.GetBkgColor (style, true));
	Font::ColorHolder txtCol (*this, _fontBox.GetTextColor (style, true));
	return SimpleTextOut (xOrg, x, y, para, start, len);
}


void PaintContext::GetTextRect (Win::Rect & rect, 
								int xOrg, 
								int x, 
								int y, 
								EditParagraph const * para, 
								int start, 
								int len)
{
	Assert (x >= 0);
	int xChar = _fontBox.GetCharWidth ();
	int colStart = (x - xOrg) / xChar;
	if (start + len > para->Len ())
		len = para->Len () - start;
	int col = _tabber.ExpandTabs (para, start, len, colStart);
	rect.left = x;
	rect.right = x + (col - colStart) * xChar;
	rect.top = y;
	rect.bottom = y + _fontBox.GetCharHeight ();
}

int PaintContext::StyledTextOut (int xOrg, 
								int x, 
								int y, 
								EditParagraph const * para, 
								int start, 
								int len)
{
	Assert (x >= 0);
	Win::TransparentBkgr bkgr (*this);
	EditStyle style = para->GetStyle ();
	Font::ColorHolder textColor (*this, _fontBox.GetTextColor (style));
	Win::Rect rect;
	GetTextRect (rect, xOrg, x, y, para, start, len);
	if (!para->IsChanged ())
	{
		SimpleTextOut (xOrg, x, y, para, start, len);
	}
	else
	{
		Brush::AutoHandle brush (_fontBox.GetBkgColor (style));
		// Revisit: untangle canvas/brush dependencies
		FillRect (rect, brush.ToNative ());
		if (style.IsRemoved ())
		{
			SimpleTextOut (xOrg, x, rect.top, para, start, len);
			// Crossed-out line
			//int y2 = (rect.bottom + rect.top + 2) / 2;
			//Pen::Black pen (*this);
			//Line (rect.left, y2, rect.right, y2);
		}
		else
		{
			SimpleTextOut (xOrg, x, y, para, start, len);
		}
	}
	return rect.right - rect.left;
}

