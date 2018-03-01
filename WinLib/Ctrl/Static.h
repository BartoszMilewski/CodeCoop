#if !defined (STATIC_H)
#define STATIC_H
//----------------------------------------------------
// (c) Reliable Software 2001 -- 2003
//----------------------------------------------------

#include "Controls.h"
#include <Graph/Icon.h>
#include <Graph/Bitmap.h>
#include <StringOp.h>

/*
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_USERITEM         0x0000000AL
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#if(WINVER >= 0x0400)
#define SS_OWNERDRAW        0x0000000DL
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL
#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL
#endif // WINVER >= 0x0400 
#if(WINVER >= 0x0501)
#define SS_REALSIZECONTROL  0x00000040L
#endif // WINVER >= 0x0501 
#define SS_NOPREFIX         0x00000080L // Don't do "&" character translation
#if(WINVER >= 0x0400)
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L
#define SS_EDITCONTROL      0x00002000L
#define SS_ENDELLIPSIS      0x00004000L
#define SS_PATHELLIPSIS     0x00008000L
#define SS_WORDELLIPSIS     0x0000C000L
#define SS_ELLIPSISMASK     0x0000C000L
#endif // WINVER >= 0x0400
*/
namespace Win
{
	class Static: public SimpleControl
	{
	public:
		Static () {}
		Static (Win::Dow::Handle winParent, int id)
			: SimpleControl (winParent, id)
		{}
	public:
		class Style : public Win::Style
		{
		public:
			enum Bits
			{
				// Add more if needed (see above)
				AlignCenter = SS_CENTER,
				Sunken = SS_SUNKEN
			};
		};
	};

	inline Win::Style & operator<< (Win::Style & style, Win::Static::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class StaticText: public Static
	{
	public:
		StaticText () {}
		StaticText (Win::Dow::Handle winParent, int id)
			: Static (winParent, id)
		{}
		std::string GetString () const
		{
			unsigned len = SendMsg (WM_GETTEXTLENGTH);
			std::string text;
			text.reserve (len + 1);
			SendMsg (WM_GETTEXT, 
				static_cast<WPARAM>(text.capacity ()), 
				reinterpret_cast<WPARAM>(writable_string (text).get_buf ()));
			return text;
		}
	};

	class StaticImage: public Static
	{
	public:
		StaticImage () {}
		StaticImage (Win::Dow::Handle winParent, int id)
			: Static (winParent, id)
		{}

		void SetIcon (Icon::Handle icon)
		{
			SendMsg (STM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(icon.ToNative ()));
		}
		Bitmap::Handle GetBitmap ()
		{
			return reinterpret_cast<HBITMAP> (SendMsg (STM_GETIMAGE, IMAGE_BITMAP));
		}
		void ReplaceBitmapPixels (Win::Color colorFrom, Win::Color colorTo = Win::Color3d::Face ())
		{
			Bitmap::Handle hbmImage = GetBitmap ();
			Assert (!hbmImage.IsNull ());
			hbmImage.ReplacePixels (colorFrom, colorTo);
		}
	};

	class StaticMaker : public ControlMaker
	{
	public:
		StaticMaker(Win::Dow::Handle winParent, int id)
			: ControlMaker("STATIC", winParent, id)
		{
		}
	};
}

#endif
