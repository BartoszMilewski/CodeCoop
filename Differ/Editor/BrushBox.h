#if !defined (BRUSHBOX_H)
#define BRUSHBOX_H
//------------------------------------
// (c) Reliable Software, 2001 -- 2002
//------------------------------------

#include "EditStyle.h"

#include <Graph/Color.h>

class BrushBox
{
public:
	BrushBox ();
	Win::Color GetBkgColor (EditStyle style, bool isHigh = false) const;
	Win::Color GetTextColor (EditStyle style, bool isHigh = false) const;

protected:
	Win::Color	_bkgColor;
	Win::Color	_textColor;
};

#endif
