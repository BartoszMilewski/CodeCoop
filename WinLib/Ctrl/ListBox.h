#if !defined (LISTBOX_H)
#define LISTBOX_H
//----------------------------------------------------
// (c) Reliable Software 2000-2004
//
//	Use ListBox::Simple ListBox::SingleSel or ListBox::MultiSel 
// (depending on whether
//	you want no-, single- or multiple- selection list).
//----------------------------------------------------

#include <Ctrl/Controls.h>
#include <StringOp.h>

namespace Win
{
	namespace ListBox
	{
		enum
		{
			NotFound = LB_ERR
		};

		typedef OwnerDraw::Handler DrawHandler;

		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				SetHasStrings = LBS_HASSTRINGS,
				SetNotify = LBS_NOTIFY,
				SetPermanentVScroll = LBS_DISABLENOSCROLL | WS_VSCROLL,
				SetNoData = LBS_NODATA | LBS_OWNERDRAWFIXED,
				SetWantKeyboardInput = LBS_WANTKEYBOARDINPUT,
				NoIntegralHeight = LBS_NOINTEGRALHEIGHT,
				// for internal use only
				SetOwnerDrawFixed = LBS_OWNERDRAWFIXED
			};
		};

		inline Win::Style & operator<< (Win::Style & style, Win::ListBox::Style::Bits bits)
		{
			style.OrIn (static_cast<Win::Style::Bits> (bits));
			return style;
		}

		class Simple : public Win::SimpleControl
		{
		public:
			Simple() {};
			Simple(Win::Dow::Handle winParent, int id)
				: Win::SimpleControl (winParent, id)
			{}
			bool HasSelChanged (int code) const
			{
				return code == LBN_SELCHANGE;
			}
			void SetHorizontalExtent (int val)
			{
				SendMsg (LB_SETHORIZONTALEXTENT, val);
			}
			void InsertItem (int idx, char const * name)
			{
				SendMsg (LB_INSERTSTRING, idx, reinterpret_cast<LPARAM> (name));
			}
			int AddItem (char const * name)
			{
				return SendMsg (LB_ADDSTRING, 0, reinterpret_cast<LPARAM> (name));
			}
			void DeleteItem (int idx)
			{
				SendMsg (LB_DELETESTRING, idx, 0);
			}
			int GetData (int idx)
			{
				return SendMsg (LB_GETITEMDATA, idx);
			}
			void SetData (int idx, int value)
			{
				SendMsg (LB_SETITEMDATA, idx, value);
			}
			int FindItemByName (char const * name) const
			{
				return SendMsg (LB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM> (name));
			}
			void Empty ()
			{
				SendMsg (LB_RESETCONTENT);
			}
			bool IsSelected (int i) const
			{
				return SendMsg (LB_GETSEL, static_cast<WPARAM> (i)) != 0;
			}
			int GetCount () const
			{
				return SendMsg (LB_GETCOUNT);
			}
			//	for owner draw with no data
			void SetCount(int count)
			{
				//  Assert LBS_NODATA style
				//	Assert not LBS_HASSTRINGS
				SendMsg(LB_SETCOUNT, count);
			}
			int ItemFromPoint(int x, int y, bool& fInsideClient) const
			{
				LRESULT lr = SendMsg(LB_ITEMFROMPOINT, 0, MAKELPARAM(x, y));
				fInsideClient = (HIWORD(lr) == 0);
				return LOWORD(lr);
			}
			void GetItemRect(int idx, Win::Rect& rc) const
			{
				SendMsg(LB_GETITEMRECT, idx, reinterpret_cast<LPARAM> (&rc));
			}
			int GetItemHeight(int idx = 0) const
			{
				return SendMsg(LB_GETITEMHEIGHT, idx);
			}
			void SetItemHeight(int height, int idx = 0)
			{
				SendMsg(LB_SETITEMHEIGHT, idx, height);
			}
			void GetItemString (int idx, std::string & str)
			{
				unsigned int len = SendMsg (LB_GETTEXTLEN, idx, 0);
				str.reserve (len + 1);
				SendMsg (LB_GETTEXT, idx, 
					reinterpret_cast<LPARAM> (writable_string (str).get_buf ()));
			}
			void ListDirectory (Win::Dow::Handle dlg, 
				char * pathBuf, 
				int idStatic = 0, // to display drive and directory
				unsigned fileTypes = DDL_DIRECTORY | DDL_DRIVES)
			{
				if (::DlgDirList (dlg.ToNative (), pathBuf, GetId (), idStatic, fileTypes) == 0)
					throw Win::Exception ("Cannot list directory", pathBuf);
			}
			// true, if directory
			bool GetSelectedPath (Win::Dow::Handle dlg,
				char * pathBuf,
				int bufLen)
			{
				return DlgDirSelectEx (dlg.ToNative (), pathBuf, bufLen, GetId ()) != 0;
			}
		};

		class SingleSel : public Simple
		{
		public:
			SingleSel () {};
			SingleSel (Win::Dow::Handle winParent, int id)
				: Simple (winParent, id)
			{}
			int Selection() const
			{
				//	Assert single selection
				return SendMsg(LB_GETCURSEL);
			}
			void Select(int idx)
			{
				//	Assert single selection
				SendMsg(LB_SETCURSEL, idx);
			}
			bool HasSelection () const { return Selection () != LB_ERR; }
		};

		class MultiSel : public Simple
		{
		public:
			MultiSel () {};
			MultiSel (Win::Dow::Handle winParent, int id)
				: Simple (winParent, id)
			{}
			void SelectAll ()
			{
				int count = GetCount ();
				SendMsg (LB_SELITEMRANGE, static_cast<WPARAM> (TRUE), MAKELPARAM (0, count));
			}
			void DeSelectAll ()
			{
				int count = GetCount ();
				SendMsg (LB_SELITEMRANGE, static_cast<WPARAM> (FALSE), MAKELPARAM (0, count));
			}
			void Select (int i)
			{
				SendMsg (LB_SETSEL, static_cast<WPARAM> (TRUE), static_cast<LPARAM> (i));
			}
			void DeSelect (int i)
			{
				SendMsg (LB_SETSEL, static_cast<WPARAM> (FALSE), static_cast<LPARAM> (i));
			}
			int GetFocusItem () const
			{
				return SendMsg (LB_GETCARETINDEX);
			}
			int GetSelCount () const
			{
				return SendMsg (LB_GETSELCOUNT);
			}
			void RetrieveSelection (std::vector<int> & result) const
			{
				int count = GetSelCount ();
				result.resize (count);
				if (count > 0)
				{
					int resCount = SendMsg (LB_GETSELITEMS, count, reinterpret_cast<LPARAM> (&result [0]));
					Assert (resCount == count);
				}
			}
		};

		class Maker : public ControlMaker
		{
		public:
			Maker(Win::Dow::Handle winParent, int id)
				: ControlMaker("LISTBOX", winParent, id)
			{}
			void SetOwnerDrawFixed (ListBox::DrawHandler * handler)
			{
				Style () << Win::ListBox::Style::SetOwnerDrawFixed;
				RegisterOwnerDraw (handler);
			}
			ListBox::Style & Style () { return static_cast<ListBox::Style &> (_style); }
		};
	}
}
#endif
