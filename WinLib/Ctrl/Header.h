#if !defined (HEADER_H)
#define HEADER_H
//----------------------------------
// (c) Reliable Software 2001 - 2007
//----------------------------------

#include "Controls.h"
#include <Graph/Canvas.h>
#include <Graph/ImageList.h>

namespace Win
{
	class Header : public SimpleControl
	{
	public:
		class Item;
		class Style;

		class Alignment
		{
		public:
			enum Type
			{
				Left = HDF_LEFT,
				Center = HDF_CENTER,
				Right = HDF_RIGHT
			};
		};

	public:
		Header (Win::Dow::Handle win = 0)
			: SimpleControl (win)
		{}

		void SetImageList (ImageList::Handle images = ImageList::Handle ())
		{
			SendMsg (HDM_SETIMAGELIST, 0, (LPARAM) images.ToNative ());
		}

		void SetColumnImage (unsigned int iCol, int iImage);
		void RemoveColumnImage (unsigned int iCol);
		void SetColumnText (unsigned iCol, char const * title);
		int AppendItem(char const *pszTitle, int width);
		void GetItemRect(int item, Win::Rect& rc) const
		{
			Header_GetItemRect(H (), item, &rc);
		}
		void SetItemWidth (int item, int width);
	};

	class Header::Style : public Win::Style
	{
	public:
		Header::Style & operator<<(void (Header::Style::*method)())
		{
			(this->*method)();
			return *this;
		}

		void Buttons()	//	SetButtons()
		{
			_style |= HDS_BUTTONS;
		}

		void DragDrop()	//	SetDragDrop()
		{
			_style |= HDS_DRAGDROP;
		}

		void FullDrag()	//	SetFullDrag()
		{
			_style |= HDS_FULLDRAG;
		}

		void FilterBar()	//	SetFilterBar()
		{
			_style |= HDS_FILTERBAR;
		}

		void Hidden()	//	SetHidden()
		{
			_style |= HDS_HIDDEN;
		}

		void Horizontal()	//	SetHorizontal()
		{
			_style |= HDS_HORZ;
		}

		void HotTracking()	//	SetHotTracking()
		{
			_style |= HDS_HOTTRACK;
		}
	};

	class HeaderMaker : public ControlMaker
	{
	public:
		HeaderMaker(Win::Dow::Handle winParent, int id)
			: ControlMaker(WC_HEADER, winParent, id)
		{
			CommonControlsRegistry::Instance()->Add(CommonControlsRegistry::LISTVIEW);
		}

		static char const *ClassName() throw()
		{
			return WC_HEADER;
		}

		Win::Header::Style & Style() { return static_cast<Win::Header::Style &> (_style); }
	};

	class Header::Item: public HDITEM
	{
	public:
		class Mask
		{
		public:
			Mask () : _mask (0) {}
			operator unsigned int () { return _mask; }
			void Format () { _mask |= HDI_FORMAT; }
			void Image () { _mask |= HDI_IMAGE; }
			void Order () { _mask |= HDI_ORDER; }
			void Bitmap () { _mask |= HDI_BITMAP; }
			void Text () { _mask |= HDI_TEXT; }
			void Width () { _mask |= HDI_WIDTH; }
			void Heigth () { _mask |= HDI_HEIGHT; }
			void Filter () { _mask |= HDI_FILTER; }
			void LParam () { _mask |= HDI_LPARAM; }
		private:
			unsigned int _mask;
		};

	public:
		Item ()
		{
			::ZeroMemory (this, sizeof (HDITEM));
		}
		Item (Win::Header & header, int colIdx, Win::Header::Item::Mask what);
		void SetImage (int imageIdx)
		{
			mask |= (HDI_IMAGE | HDI_FORMAT);
			fmt |= (HDF_BITMAP_ON_RIGHT | HDF_IMAGE);
			iImage = imageIdx;
		}
		void ResetImage ()
		{
			mask |= HDI_FORMAT;
			fmt &= ~HDF_IMAGE;
			fmt &= ~HDF_BITMAP_ON_RIGHT;
		}
		void SetText (char const * title)
		{
			mask |= (HDI_TEXT | HDI_FORMAT);
			fmt |= HDF_STRING;
			pszText = const_cast<char *> (title);
			cchTextMax = strlen (title) + 1;
		}
		void SetWidth (int width)
		{
			mask |= HDI_WIDTH;
			cxy = width;
		}
#if 0
		// These are only available in v. 6 of Common Controls
		void SortDown ()
		{
			Assert ((fmt & HDF_IMAGE) == 0);
			Assert ((fmt & HDF_BITMAP) == 0);
			mask |= HDI_FORMAT;
			fmt |= HDF_SORTDOWN;
		}
		void SortUp ()
		{
			Assert ((fmt & HDF_IMAGE) == 0);
			Assert ((fmt & HDF_BITMAP) == 0);
			mask |= HDI_FORMAT;
			fmt |= HDF_SORTUP;
		}
#endif
	};
}

#endif