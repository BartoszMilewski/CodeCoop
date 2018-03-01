#if !defined (IMAGELIST_H)
#define IMAGELIST_H
//----------------------------------------------------
// ImageList.h
// (c) Reliable Software 2000 -- 2003
//
//----------------------------------------------------

#include <Win/Handles.h>
#include <Graph/Brush.h>
#include <Graph/Icon.h>
#include <Bit.h>

namespace ImageList
{
	class Flags : public BitFieldMask<DWORD>
	{
	public:
		Flags (unsigned long value = 0)
			: BitFieldMask<DWORD> (value)
		{}
		void SetDefaultColor () { Set (ILC_COLOR); }
		void SetColor4 () { Set (ILC_COLOR4); }
		void SetColor8 () { Set (ILC_COLOR8); }
		void SetColor16 () { Set (ILC_COLOR16); }
		void SetColor24 () { Set (ILC_COLOR24); }
		void SetColor32 () { Set (ILC_COLOR32); }
		void SetDeviceColor () { Set (ILC_COLORDDB); }
		void SetMask () { Set (ILC_MASK); }
	};

	// Use to make background transparent
	class FlagsMasked: public Flags
	{
	public:
		FlagsMasked ()
			: Flags (ILC_MASK)
		{}
	};

	class Flags256ColorsMasked: public Flags
	{
	public:
		Flags256ColorsMasked ()
			: Flags (ILC_COLOR8 | ILC_MASK)
		{}
	};

	class Handle: public Win::Handle<HIMAGELIST>
	{
	public:
		Handle (HIMAGELIST	h = NullValue ()) : Win::Handle<HIMAGELIST> (h) {}
		// returns icon index
		int AddIcon (Icon::Handle icon)
		{
			return ::ImageList_AddIcon (H (), icon.ToNative ());
		}
		int ReplaceIcon (int idx, Icon::Handle icon)
		{
			return ::ImageList_ReplaceIcon (H (), idx, icon.ToNative ());
		}
		// Make image at index imageIdx an overlay to be addressed by
		// overlayIdx (which must be > 0 -- 0 means no overlay)
		void SetOverlayImage (int imageIdx, int overlayIdx)
		{
			Assert (overlayIdx != 0);
			::ImageList_SetOverlayImage (H (), imageIdx, overlayIdx);
		}
		void GetImageSize (int & width, int & height);
		// if increasing the count, add new images using Replace
		void SetCount (int newCount)
		{
			::ImageList_SetImageCount (H (), newCount);
		}
		int GetCount ()
		{
			return ::ImageList_GetImageCount (H ());
		}
		AutoHandle Duplicate ();
	};

	class AutoHandle: public Win::AutoHandle<ImageList::Handle>
	{
	public:
		AutoHandle (int imageWidth, int imageHeight,
					int count, 
					ImageList::Flags flags = ImageList::Flags256ColorsMasked (),
					int grow = 0);

		// By default, make grey #c0c0c0 transparent
		AutoHandle (Win::Instance inst, int bitmapId, int imageWidth, 
					Win::Color mask = Win::Color (0xc0, 0xc0, 0xc0), int growCount = 0);

		AutoHandle (HIMAGELIST h = 0)
			: Win::AutoHandle<ImageList::Handle> (h)
		{}
	};
}

#endif
