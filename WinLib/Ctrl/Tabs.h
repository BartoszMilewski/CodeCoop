#if !defined (TABS_H)
#define TABS_H
//-------------------------------------------
// (c) Reliable Software 1998-2002
//-------------------------------------------

#include <Ctrl/Controls.h>
#include <Win/Notification.h>
#include <Graph/Font.h>
#include <Graph/ImageList.h>

namespace Tab
{
	class Item: public TCITEM
	{
	public:
		Item (char const * text)
		{
			::ZeroMemory (this, sizeof (TCITEM));
			mask = TCIF_TEXT | TCIF_IMAGE;
			iImage = -1;
			pszText = const_cast<char *>(text);
		}
		void SetText (char const * text)
		{
			pszText = const_cast<char *>(text);
		}
		void SetParam (long param)
		{
			mask |= TCIF_PARAM;
			lParam = param;
		}
	};

	class Handle: public Win::ControlWithFont
	{
	public:
		Handle (Win::Dow::Handle winParent, int id)
			: Win::ControlWithFont (winParent, id)
		{}
		Handle (Win::Dow::Handle win = Win::Dow::Handle ())
			: Win::ControlWithFont (win)
		{}

		int GetCount () const;
		int AddTab (int cmdId, char const * caption, long param = -1);
		int AddTab (int cmdId, Tab::Item const & tabItem);
		void DeleteTab (int itemIdx);
		void Clear ()
		{
			TabCtrl_DeleteAllItems (H ());
		}
		void SetImage (int itemIdx, int imageIdx);
		void RemoveImage (int itemIdx);
		void ReSize (int left, int top, int width, int height)
		{
			Move (left, top, width, height);
		}
		int GetCurSelection () const
		{
			return SendMsg (TCM_GETCURSEL);
		}
		void SetCurSelection (int itemIdx)
		{
			SendMsg (TCM_SETCURSEL, itemIdx);
		}
		long GetParam (int itemIdx) const;

		template<class E>
		E GetParam (int itemIdx) const
		{
			return static_cast<E> (GetParam (itemIdx));
		}
		void SetImageList (ImageList::Handle images = ImageList::Handle ())
		{
			SendMsg (TCM_SETIMAGELIST, 0, (LPARAM) images.ToNative ());
		}
		// Given window rectangle in which tabs and page must fit
		// return display area for application to use (below tabs)
		void GetDisplayArea (Win::Rect & bigRectangle);
		// Given display area below tabs, get window rectangle
		// in which both tabs and dispaly area will fit
		void GetWindowRect (Win::Rect & displayArea);
		void AdjustRectangleUp (Win::Rect & rect)
		{
			TabCtrl_AdjustRect (H (), TRUE, &rect);
		}
		void AdjustRectangleDown (Win::Rect & rect)
		{
			TabCtrl_AdjustRect (H (), FALSE, &rect);
		}
	protected:
		bool SetItem (int itemIdx, TC_ITEM * item)
		{
			return SendMsg (TCM_SETITEM, itemIdx, (LPARAM) item) != 0;
		}
		bool GetItem (int itemIdx, TC_ITEM * item) const
		{
			return SendMsg (TCM_GETITEM, itemIdx, (LPARAM) item) != 0;
		}
	};

	class Style: public Win::Style
	{
	public:
		enum Bits
		{
			MultiLine = TCS_MULTILINE,
			ToolTips = TCS_TOOLTIPS
		};
	};

	inline Win::Style & operator<<(Win::Style & style, Tab::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class Maker : public Win::ControlMaker
	{
	public:
		Maker (Win::Dow::Handle winParent, unsigned ctrlId)
			: Win::ControlMaker (WC_TABCONTROL, winParent, ctrlId)
		{
			Style () << Win::Style::ClipChildren << Win::Style::ClipSiblings;
		}
		Tab::Style & Style () { return static_cast<Tab::Style &> (_style); }
		Tab::Handle Create ()
		{
			return Tab::Handle (Win::ControlMaker::Create ());
		}
	};
}

// Tab control that uses enumeration to address tabs

template<class E>
class EnumeratedTabs: public Tab::Handle
{
public:
	EnumeratedTabs (Win::Dow::Handle parentWin, int id)
	{
		Tab::Maker tabMaker (parentWin, id);
		Reset (tabMaker.Create ());
	}
    E GetSelection () const
	{
		int idx = GetCurSelection ();
		return GetParam<E> (idx);
	}
	void SetSelection (E page)
	{
		SetCurSelection (PageToIndex (page));
	}
	void InsertPageTab (E page, std::string const & title)
	{
		AddTab (0, title.c_str (), page);
	}
	void RemovePageTab (E page)
	{
		int idx = PageToIndex (page);
		if (idx != -1)
			DeleteTab (idx);
	}
protected:
	// Returns -1 if not found
	int PageToIndex (E viewPage) const
	{
		int count = GetCount ();
		int idx = 0;
		while (idx < count && GetParam<E> (idx) != viewPage)
			++idx;
		return (idx < count)? idx: -1;
	}
};

namespace Notify
{
	class TabHandler : public Handler
	{
	public:
		explicit TabHandler (unsigned id) : Handler (id) {}
		virtual bool OnSelChange () throw ()
			{ return false; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}

#endif
