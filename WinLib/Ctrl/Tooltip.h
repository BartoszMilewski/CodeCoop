#if !defined (TOOLTIP_H)
#define TOOLTIP_H
//----------------------------------
// (c) Reliable Software 2003 - 2005
//----------------------------------

#include <Win/Instance.h>
#include <Win/Notification.h>

namespace Tool
{
	// ToolTip is send by Windows when tip text is required
	class Tip : public TOOLTIPTEXT
	{
	public:
		bool IsIdFrom () const { return (uFlags & TTF_IDISHWND) == 0; }
		bool IsHwndFrom () const { return (uFlags & TTF_IDISHWND) != 0; }
		void CopyText (char const * text)
		{
			strncpy (szText, text, sizeof (szText));
		}
		// Warning: text cannot be a temporary string
		void SetText (char const * text)
		{
			lpszText = const_cast<char *> (text);
		}
		void SetText (Win::Instance hInst, int resStringId)
		{
			lpszText = MAKEINTRESOURCE (resStringId);
			hinst = hInst;
		}
		char const * GetTipText () const { return lpszText; }
	};

	class TipForCtrl: public Tip
	{
	public:
		int IdFrom () const
		{
			Assert (IsIdFrom ());
			return hdr.idFrom;
		}
	};

	class TipForWindow: public Tip
	{
	public:
		Win::Dow::Handle WinFrom () const 
		{
			Assert (IsHwndFrom ());
			return reinterpret_cast<HWND> (hdr.idFrom);
		}
	};
}

namespace Notify
{
	class ToolTipHandler : public Notify::Handler
	{
	public:
		explicit ToolTipHandler (unsigned id) : Notify::Handler (id) {}
		virtual bool OnNeedText (Tool::TipForWindow * tip) throw ()
			{ return false; }
		virtual bool OnNeedText (Tool::TipForCtrl * tip) throw ()
			{ return false; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}


#endif
