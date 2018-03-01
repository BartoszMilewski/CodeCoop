#if !defined BITINFO_H
#define BITINFO_H
// -----------------------------
// (c) Reliable Software 2000-03
// -----------------------------

namespace Bitmap
{
	class FileHeader: public BITMAPFILEHEADER
	{
	public:
		FileHeader ()
		{
			bfReserved1 = 0; 
			bfReserved2 = 0; 
			bfType = MAKEWORD ('B', 'M');
		}

		void Init (int sizeFile, int offBits)
		{
			bfSize = sizeFile; 
			bfOffBits = offBits;
		}

		void Init (BITMAPFILEHEADER * pHeader)
		{
			memcpy (this, pHeader, sizeof (BITMAPFILEHEADER));
		}

		BOOL IsBitmap () const
		{
			return bfType == MAKEWORD ('B', 'M');
		}

		int AllSize () const { return bfSize; }

		int OffsetBits () const { return bfOffBits; }
	};

	class Core: public BITMAPCOREINFO
	{
	public:
		int Size () const { return bmciHeader.bcSize; }
		int Width () const { return bmciHeader.bcWidth; }
		int Height () const { return bmciHeader.bcHeight; }
		int Planes () const { return bmciHeader.bcPlanes; }
		int BitsPerPixel () const { return bmciHeader.bcBitCount; }
		int NumColors () const; 
	};

	class InfoHeader: public BITMAPINFOHEADER
	{
	public:
		InfoHeader (int width, int height, int bitsPerPixel)
		{
			Init (width, height, bitsPerPixel);
		}
		void Init (int width, int height, int bitsPerPixel)
		{
			biSize = sizeof (BITMAPINFOHEADER);
			biWidth = width; 
			biHeight = height; 
			biPlanes = 1;
			Assert (bitsPerPixel == 1
				||  bitsPerPixel == 4
				||  bitsPerPixel == 8
				||  bitsPerPixel == 24);
			biBitCount = bitsPerPixel; 
			biCompression = BI_RGB; // no compression
			biSizeImage = 0; 
			biXPelsPerMeter = 0; 
			biYPelsPerMeter = 0; 
			biClrUsed = 0;		// all used
			biClrImportant = 0; // all colors important
		}
	};

	class Info: public BITMAPINFO
	{
	public:
		void InitMono (int width, int height) // Monochrome
		{
			InfoHeader * infoHeader = static_cast<InfoHeader *> (&bmiHeader);
			infoHeader->Init (width, height, 1);
		}
		void InitTrue (int width, int height) // True Color
		{
			InfoHeader * infoHeader = static_cast<InfoHeader *> (&bmiHeader);
			infoHeader->Init (width, height, 24);
		}
		void Init (BITMAPINFO * pInfo, int cColors);
		void Init (BITMAPCOREINFO * pInfo, int cColors);

		int Size () const           { return bmiHeader.biSize; }
		int Width () const          { return bmiHeader.biWidth; }
		int Height () const         { return bmiHeader.biHeight; }
		int Planes () const         { return bmiHeader.biPlanes; }
		int BitsPerPixel () const   { return bmiHeader.biBitCount; }
		BOOL IsCompressed () const  { return bmiHeader.biCompression != BI_RGB; }
		int ColorsUsed () const     { return bmiHeader.biClrUsed; }
		void SetColor (int i, RGBQUAD rgbQ)
		{
			Assert (i < NumColors ());
			bmiColors [i] = rgbQ;
		}

		RGBQUAD GetColor (int i) const { return bmiColors [i]; }
		int NumColors () const; 
		int ColorsImportant () const { return bmiHeader.biClrImportant; }
	};
}

#endif
