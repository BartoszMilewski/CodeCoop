#if !defined (BITMAP_H)
#define BITMAP_H
//---------------------------
// (c) Reliable Software 1998
//---------------------------
#include <Graph/BitInfo.h>
#include <Graph/Canvas.h>
#include <Graph/Holder.h>
#include <Win/GdiHandles.h>
#include <Win/Instance.h>

namespace Bitmap
{
	class Handle : 	public Win::Handle<HBITMAP>
	{
	public:
		Handle (HBITMAP h = 0)
			: Win::Handle<HBITMAP> (h)
		{}
		void ReplacePixels (Win::Color colorFrom, Win::Color colorTo);
	};

	class Logical
	{
	public:
		Logical (Bitmap::Handle h)
		{
			::GetObject (h.ToNative (), sizeof (BITMAP), &_bmp);
		}
		int Width () const { return _bmp.bmWidth; }
		int Height () const { return _bmp.bmHeight; }
	private:
		BITMAP _bmp;
	};

	class AutoHandle: public Win::AutoHandle<Bitmap::Handle, Gdi::Disposal<Bitmap::Handle> >
	{
		typedef Win::AutoHandle<Handle, Gdi::Disposal<Bitmap::Handle> > Base;
	public:
		AutoHandle () {}
		// Revisit: move these to Maker
		void Load (Win::Instance hInst, int id);
	    void Load (Win::Instance hInst, char const * name);
	protected:
		AutoHandle (HBITMAP h) : Base (h) {}
	};

	// Revisit: encapsulate DIBSECTION, use GetObject to initialize it
	class Section: public AutoHandle
	{
	public:
		Section (Win::Canvas canvas, Bitmap::Info const * info)
			: AutoHandle (::CreateDIBSection (canvas.ToNative (), info, DIB_RGB_COLORS, &_bits, 0, 0))
		{
		}
		void * GetBits () { return _bits; }
	private:
		void * _bits;
	};

	class Compatible : public AutoHandle
	{
	public:
		Compatible (Win::Canvas canvas, int width, int height)
			: AutoHandle (::CreateCompatibleBitmap (canvas.ToNative (), width, height))
		{}
	};

	class Monochrome : public AutoHandle
	{
	public:
		Monochrome (int width, int height)
			: AutoHandle (::CreateBitmap (width, height, 1, 1, 0))
		{}
	};

	typedef Gdi::ObjectHolder<Bitmap::Handle> Holder;
}

#endif
