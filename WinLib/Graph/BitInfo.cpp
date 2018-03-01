// --------------------------
// (c) Reliable Software 2000
// --------------------------
#include <WinLibBase.h>
#include "BitInfo.h"

namespace Bitmap
{
	int Info::NumColors () const 
	{
		int bpp = BitsPerPixel ();

		if (bpp == 1)
			return 2;
		if (ColorsUsed () != 0)
			return ColorsUsed ();

		int colors = 0;
		switch (bpp)
		{
		case 4:
			colors = 16;
			break;
		case 8:
			colors = 256;
			break;
		case 16:
		case 24:
		case 32:
			colors = 0; // The value is null
			break;
		default:
			throw ("Unknown bitmap format");
		}
		return colors;
	}

	int Core::NumColors () const 
	{
		int colors;
		switch (BitsPerPixel ())
		{
		case 1:
			colors = 2;
			break;
		case 4:
			colors = 16;
			break;
		case 8:
			colors = 256;
			break;
		case 16:
		case 24:
		case 32:
			colors = 0; // The value is null
			break;
		default:
			throw ("Unknown bitmap format");
		}
		return colors;
	}

	void Info::Init (BITMAPINFO * pInfo, int cColors)
	{
		memcpy (this, pInfo, sizeof (BITMAPINFOHEADER));
		if (Planes () != 1)
			throw ("Multiple bit planes not supported");
		for (int i = 0; i < cColors; i++)
		{
			bmiColors [i] = pInfo->bmiColors [i];
		}
	}

	void Info::Init (BITMAPCOREINFO * pInfo, int cColors)
	{
		// convert it to Info Header
		bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
		bmiHeader.biWidth = pInfo->bmciHeader.bcWidth; 
		bmiHeader.biHeight = pInfo->bmciHeader.bcHeight; 
		bmiHeader.biPlanes = pInfo->bmciHeader.bcPlanes; 
		bmiHeader.biBitCount = pInfo->bmciHeader.bcBitCount; 
		bmiHeader.biCompression = BI_RGB; 
		bmiHeader.biSizeImage = 0; 
		bmiHeader.biXPelsPerMeter = 0; 
		bmiHeader.biYPelsPerMeter = 0; 
		bmiHeader.biClrUsed = 0; 
		bmiHeader.biClrImportant = 0;

		if (Planes () != 1)
			throw ("Multiple bit planes not supported");

		for (int i = 0; i < cColors; i++)
		{
			bmiColors [i].rgbRed   = pInfo->bmciColors [i].rgbtRed;
			bmiColors [i].rgbGreen = pInfo->bmciColors [i].rgbtGreen;
			bmiColors [i].rgbBlue  = pInfo->bmciColors [i].rgbtBlue;
			bmiColors [i].rgbReserved = 0;
		}
	}
}