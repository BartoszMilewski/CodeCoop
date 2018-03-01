// (c) Reliable Software, 2001
#include "precompiled.h"
#include "BrushBox.h"
#include <Graph/Brush.h>
#include <Graph/Pen.h>
#include <Graph/CanvTools.h>
#include <Dbg/Assert.h>

BrushBox::BrushBox ()
{
	_bkgColor = Win::Color::Window ();
	int intensity = _bkgColor.R () + _bkgColor.G () + _bkgColor.B ();
	if (intensity > 3 * 128)
		_textColor = Win::Color (0, 0, 0);
	else
		_textColor = Win::Color (255, 255, 255);
}

Win::Color BrushBox::GetBkgColor (EditStyle style, bool isHigh) const
{
	if (isHigh)
		return _textColor;

	// use scale from 0 to 5
	// insert 5  
	// paste  4   mov
	// cut    3   mov rm
	// delete 2       rm
	bool rm = style.IsRemoved ();
	bool mov = style.IsMoved ();
	int intensity;
	if (rm && mov)
		intensity = 3;
	else if (rm && !mov)
		intensity = 2;
	else if (!rm && mov)
		intensity = 4;
	else
		intensity = 5;

	int shade = (intensity + 1) / 2;
	int red;
	int green;
	int blue;
	switch (style.GetChangeSource ())
	{
	case EditStyle::chngHistory:	// Historical change
		red   = shade;
		green = intensity;
		blue  = intensity;
		break;
	case EditStyle::chngUser:		// User change
		red   = intensity;
		green = intensity;
		blue  = shade;
		break;
	case EditStyle::chngSynch:		// Synch change
		red   = intensity;
		green = shade;
		blue  = intensity;
		break;
	case EditStyle::chngMerge:		// Merge change
		red   = intensity - 1;
		green = intensity - 1;
		blue  = intensity - 1;
		break;
	default:
		Assert (!"Bad style passed to BrushBox");
	}

	// 5 * 51 = 255
	return Win::Color (red * 51, green * 51, blue * 51);
}


Win::Color BrushBox::GetTextColor (EditStyle style, bool isHigh) const
{
	if (isHigh)
		return _bkgColor;

	if (style.IsRemoved ())
		return Win::Color (255, 255, 255);

	if (style.IsChanged ())
		return Win::Color (0, 0, 0); // changed but not removed

	return _textColor;
}