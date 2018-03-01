//  (c) Reliable Software, 2001-03
#include <WinLibBase.h>
#include <Graph/Bitmap.h>
#include <Graph/Canvas.h>
#include <Graph/Font.h>
#include <Graph/Brush.h>

using namespace Bitmap;

void Handle::ReplacePixels (Win::Color colorFrom, Win::Color colorTo)
{
    Bitmap::Logical bitmapInfo (*this);

	//	create a canvas to work on ourself
	Win::MemCanvas canvas (0);
	Bitmap::Holder holder (canvas, *this);

	//	create monochrome bitmap mask
	Win::MemCanvas canvasMask (0);
	Bitmap::Monochrome hbmMask (bitmapInfo.Width (), bitmapInfo.Height ());
	Bitmap::Holder maskHolder (canvasMask, hbmMask);

	//	when converting color to monochrome
	//		background color -> 1
	//		other colors	 -> 0
	{
		Font::BkgHolder bkgHolder (canvas, colorFrom);
		::BitBlt (canvasMask.ToNative (), 0, 0, bitmapInfo.Width (), bitmapInfo.Height (),
				  canvas.ToNative (), 0, 0, SRCCOPY);
		//	canvasMask now has 1's wherever colorFrom was
	}

	//	when converting monochrome to color
	//		0 -> target canvas text color
	//		1 -> target canvas background color
	//	by setting target canvas background to 0 and text of RGB(0xff, 0xff, 0xff)
	//	canvasMask can be used to mask out pixels that are 1 in the mask
	//	(and which which are colorFrom in ourself)
	{
		Font::BkgHolder bkgHolder (canvas, Win::Color (0, 0, 0));
		Font::ColorHolder colorHolder (canvas, Win::Color (0xff, 0xff, 0xff));
		::BitBlt (canvas.ToNative (), 0, 0, bitmapInfo.Width (), bitmapInfo.Height (),
				  canvasMask.ToNative (), 0, 0, SRCAND);
	}

	//	finally, by setting target canvas background to colorTo and text to 0
	//	we can OR in canvasMask to set the what 1 in the mask to colorTo in
	//	ourself (while leaving the rest untouched)
	{
		Font::ColorHolder colorHolder (canvas, Win::Color (0, 0, 0));
		Font::BkgHolder bkgHolder (canvas, colorTo);
		::BitBlt (canvas.ToNative (), 0, 0, bitmapInfo.Width (), bitmapInfo.Height (),
				  canvasMask.ToNative (), 0, 0, SRCPAINT);
	}
}

void AutoHandle::Load (Win::Instance hInst, char const * name)
{
	Assert (IsNull ());
	Handle::Reset (::LoadBitmap (hInst, name));
}

void AutoHandle::Load (Win::Instance hInst, int id)
{
	Assert (IsNull ());
	Handle::Reset (::LoadBitmap (hInst, MAKEINTRESOURCE (id)));
}
