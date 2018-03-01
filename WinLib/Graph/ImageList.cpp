// (c) Reliable Software 2002
#include <WinLibBase.h>
#include <Graph/ImageList.h>

template<>
void Win::Disposal<ImageList::Handle>::Dispose (ImageList::Handle h) throw ()
{
	::ImageList_Destroy (h.ToNative ());
}

namespace ImageList
{
	void Handle::GetImageSize (int & width, int & height)
	{
		IMAGEINFO info;
		ImageList_GetImageInfo (H (), 0, &info);
		Win::Rect rect (info.rcImage);
		width = rect.Width ();
		height = rect.Height ();
	}

	AutoHandle::AutoHandle (int imageWidth, int imageHeight, int count, ImageList::Flags flags, int grow)
		: Win::AutoHandle<ImageList::Handle> 
				(::ImageList_Create (imageWidth, imageHeight, flags, count, grow))
	{
		if (IsNull ())
			throw Win::Exception ("Internal error: Couldn't create image list.");
		Win::ClearError ();
	}

	AutoHandle::AutoHandle (Win::Instance inst, int bitmapId, int imageWidth, Win::Color mask, int growCount)
		: Win::AutoHandle<ImageList::Handle>
		(::ImageList_LoadBitmap (inst, MAKEINTRESOURCE (bitmapId), imageWidth, growCount, mask.ToNative ()))
	{
		if (IsNull ())
			throw Win::Exception ("Internal error: Couldn't load image list.");
		Win::ClearError ();
	}


	AutoHandle Handle::Duplicate ()
	{
		return AutoHandle (ImageList_Duplicate (H ()));
	}
}
