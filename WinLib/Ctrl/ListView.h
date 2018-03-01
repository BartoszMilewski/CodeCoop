#if !defined LISTVIEW_H
#define LISTVIEW_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <Ctrl/Controls.h>
#include <Win/Notification.h>
#include <Graph/Color.h>
#include <Graph/ImageList.h>
#include <Win/Message.h>
#include <Ctrl/Header.h>

namespace Win
{
	// Base class for other types of ListViews
	// ListView can be filled only using specialized subclasses
	class ListView : public Win::SimpleControl
	{
	public:
		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				MakeReport = LVS_REPORT,
				ShowSelAlways = LVS_SHOWSELALWAYS,
				EditLabels = LVS_EDITLABELS,
				SingleSelection = LVS_SINGLESEL
			};

			class Ex: public Win::Style::Ex
			{
			public:
				enum Bits
				{
					FullRowSelect = LVS_EX_FULLROWSELECT,
					LabelTip = LVS_EX_LABELTIP
				};
			};
		};
	public:
		// ItemState is an argument to some methods of ListNotifyHandler
		class ItemState
		{
		public:
			ItemState (NMHDR * hdr)
				: _state (reinterpret_cast<NMLISTVIEW *>(hdr))
			{}
			bool WasSelected () const { return (_state->uOldState & LVIS_SELECTED) != 0; }
			bool IsSelected () const { return (_state->uNewState & LVIS_SELECTED) != 0; }
			bool GainedSelection () const { return !WasSelected () && IsSelected (); }
			bool LostSelection () const { return WasSelected () && !IsSelected (); }
			int Idx () const { return _state->iItem; }
			int GetOldStateImageIdx () const { return ((_state->uOldState & LVIS_STATEIMAGEMASK) >> 12); }
			int GetNewStateImageIdx () const { return ((_state->uNewState & LVIS_STATEIMAGEMASK) >> 12); }

		private:
			// NMHDR hdr;
			// int   iItem; 
			// int   iSubItem; 
			// UINT  uNewState; 
			// UINT  uOldState; 
			// UINT  uChanged; 
			// POINT ptAction; 
			// LPARAM lParam;
			NMLISTVIEW * _state;
		};

		// Item is an argument to some methods of ListNotifyHandler
		class Item : public LVITEM
		{
			// UINT mask; 
			// int iItem; 
			// int iSubItem; 
			// UINT state; 
			// UINT stateMask; 
			// LPTSTR pszText; 
			// int cchTextMax; 
			// int iImage; 
			// LPARAM lParam;
			// int iIndent;		// _WIN32_IE >= 0x0300
			// int iGroupId;	// _WIN32_IE >= 0x560 (including what follows)
			// UINT cColumns;	// tile view columns
			// PUINT puColumns;
		public:
			Item ()
			{
				::ZeroMemory (this, sizeof (Item));
			}
			void Unmask ()
			{
				mask = 0;
				stateMask = 0;
				state = 0;
			}
			int GetIdx () const { return iItem; }
			int GetSubItem () const { return iSubItem; }
			// For callback items
			void SetPos (int pos) { iItem = pos; }
			void SetSubItem (int col) { iSubItem = col; }
			void SetParam (int data)
			{
				mask |= LVIF_PARAM;
				lParam = data;
			}
			void SetText (char const * text)
			{
				mask |= LVIF_TEXT; 
				pszText = const_cast<char *>(text);
				cchTextMax = strlen (text); 
			}
			// Use these three together to copy text into Item buffer
			int GetTextLen () const { return cchTextMax; }
			char * GetBuf () const { return pszText; }
			void SetText () { mask |= LVIF_TEXT; }

			void SetIcon (int iIcon)
			{
				mask |= LVIF_IMAGE;
				iImage = iIcon;
			}
			void SetOverlay (int iOverlay = 0)
			{
				Assert (iOverlay <= 15);
				mask |= LVIF_STATE;
				stateMask |= LVIS_OVERLAYMASK;
				state |= INDEXTOOVERLAYMASK (iOverlay);
			}
		};

		class Request
		{
		public:
			Request (ListView::Item const & item)
				: _mask (item.mask)
			{}
			bool IsText () const { return (_mask & LVIF_TEXT) != 0; }
			bool IsIcon () const { return (_mask & LVIF_IMAGE) != 0; }
			bool IsParam () const { return (_mask & LVIF_PARAM) != 0; }
			bool IsState () const { return (_mask & LVIF_STATE) != 0; }
		private:
			unsigned _mask;
		};

		class State
		{
		public:
			State (ListView::Item const & item)
				: _mask (item.stateMask),
				  _state (item.state)
			{}
		private:
			unsigned _mask;
			unsigned _state;
		};

		// Internal class, not used by client
		class ItemManipulator: public ListView::Item
		{
		public:
			void Init (int pos, char const * text, int data = 0);
			void MakeCallback (int maxLen)
			{
				// We use only those two callbacks; Other posibilities are: LVIF_PARAM, LVIF_STATE
				mask = LVIF_TEXT | LVIF_IMAGE; 
				pszText = LPSTR_TEXTCALLBACK; 
				cchTextMax = maxLen; 
				iImage = I_IMAGECALLBACK;
			};
			void PrepareForRead (int pos, char * buf, int bufLen, int subItem = 0)
			{
				// We use only those two; Other possibilities are: LVIF_PARAM, LVIF_STATE
				mask = LVIF_TEXT | LVIF_PARAM; 
				iItem = pos; 
				iSubItem = subItem; 
				pszText = buf;
				cchTextMax = bufLen; 
			}
			void PrepareForRead (int pos, int subItem = 0)
			{
				mask = LVIF_PARAM; 
				iItem = pos; 
				iSubItem = subItem; 
			}
		};

		class CustomDraw : public NMLVCUSTOMDRAW
		{
		public:
			static unsigned const DoDefault = CDRF_DODEFAULT;				// The control will draw itself.
																			// It will not send additional NM_CUSTOMDRAW notifications for this paint cycle.
			static unsigned const NotifyItemDraw = CDRF_NOTIFYITEMDRAW;		// The control will notify the parent of any item-specific drawing operations.
																			// It will send NM_CUSTOMDRAW notification messages before and after it draws items.
			static unsigned const NotifyPostErase = CDRF_NOTIFYPOSTERASE;	// The control will notify the parent after erasing an item.
			static unsigned const NotifyPostPaint = CDRF_NOTIFYPOSTPAINT;	// The control will send an NM_CUSTOMDRAW notification when the painting cycle
																			// for the entire control is complete.
			static unsigned const NewFont = CDRF_NEWFONT;					// Your application specified a new font for the item; the control will use the new font.
			static unsigned const NotifySubItemDraw = CDRF_NOTIFYSUBITEMDRAW;//Your application will receive an NM_CUSTOMDRAW message with dwDrawStage set to
																			// CDDS_ITEMPREPAINT | CDDS_SUBITEM before each list-view subitem is drawn.
																			// You can then specify font and color for each subitem separately or return CDRF_DODEFAULT
																		    // for default processing.
			static unsigned const SkipDefaultDraw = CDRF_SKIPDEFAULT;		// The control will not perform any painting at all.

			bool IsPrePaint () const { return nmcd.dwDrawStage == CDDS_PREPAINT; }
			bool IsPostPaint () const { return nmcd.dwDrawStage == CDDS_POSTPAINT; }
			bool IsPreErase () const { return nmcd.dwDrawStage == CDDS_PREERASE; }
			bool IsPostErase () const { return nmcd.dwDrawStage == CDDS_POSTERASE; }

			bool IsItemPrePaint () const { return nmcd.dwDrawStage == CDDS_ITEMPREPAINT; }
			bool IsItemPostPaint () const { return nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT; }
			bool IsItemPreErase () const { return nmcd.dwDrawStage == CDDS_ITEMPREERASE; }
			bool IsItemPostErase () const { return nmcd.dwDrawStage == CDDS_ITEMPOSTERASE; }

			bool IsSubItem () const { return (nmcd.dwDrawStage & CDDS_SUBITEM) != 0; }

			unsigned int GetItemIdx () const { return nmcd.dwItemSpec; }
			Win::Canvas GetItemCanvas () const { return Win::Canvas (nmcd.hdc); }

			void SetTextColor (Win::Color color)
			{
				clrText = color.ToNative ();
			}
		};

	public:
		// ListView methods
		void SetBkColor (Win::Color bkColor)
		{
			ListView_SetBkColor (H (), bkColor.ToNative ());
			ListView_SetTextBkColor (H (), bkColor.ToNative ());
		}

		void ReSize (int left, int top, int width, int height)
		{
			Move (left, top, width, height);
		}

		// Scrolling
		void LineUp ()
		{
			SendMsg (WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0));
		}

		void LineDown ()
		{
			SendMsg (WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0));
		}

		void ScrollTop ()
		{
			SendMsg (WM_VSCROLL, MAKEWPARAM (SB_TOP, 0));
		}

		void ScrollBottom ()
		{
			SendMsg (WM_VSCROLL, MAKEWPARAM (SB_BOTTOM, 0));
		}

		void ClearRows ()
		{
			if (!ListView_DeleteAllItems (H ()))
				throw Win::Exception ("Internal error: Could not clear listview.");
			_count = 0;
		}

		int GetCount () const
		{
			Assert (_count == ListView_GetItemCount (H ()));
			return _count;
		}

		void SetFocus ()
		{
			Dow::Handle::SetFocus ();
		}

		void SetFocus (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, LVIS_FOCUSED, LVIS_FOCUSED);
		}

		void RemoveFocus (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, 0, LVIS_FOCUSED);
		}

		void SetHilite (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, LVIS_DROPHILITED, LVIS_DROPHILITED);
		}

		void RemoveHilite (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, 0, LVIS_DROPHILITED);
		}

		void SetImageList (ImageList::Handle images = ImageList::Handle ())
		{
			ListView_SetImageList(H (), images.ToNative (), LVSIL_SMALL);
		}

		void Select (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, LVIS_SELECTED, LVIS_SELECTED);
		}

		void SelectAll ()
		{
			int count = GetCount ();
			for (int i = 0; i < count; i++)
				Select (i);
		}

		void DeSelect (int i)
		{
			Assert (i < _count);
			ListView_SetItemState (H (), i, 0, LVIS_SELECTED);
		}

		void DeSelectAll ()
		{
			int count = GetCount ();
			for (int i = 0; i < count; i++)
				DeSelect (i);
		}

		void SetItem (ListView::Item & item)
		{
		    ListView_SetItem (H (), &item);
		}

		void RemoveItem (int i)
		{
			Assert (i < _count);
			_count--;
			Assert (_count >= 0);
			ListView_DeleteItem (H (), i);
		}

		void UpdateItem (int i)
		{
			Assert (i < _count);
			ListView_Update(H (), i);
		}

		void RedrawItems (int iFirst, int iLast)
		{
			Assert (iFirst >= 0);
			Assert (iLast < _count);
			ListView_RedrawItems (H (), iFirst, iLast);
			Update ();
		}

		void DelayRedrawItem (int item)
		{
			Assert (item >= 0);
			Assert (item < _count);
			ListView_RedrawItems (H (), item, item);
		}

		void Repaint ()
		{
			Update ();
		}

		void EnsureVisible (int item)
		{
			Assert (item < _count);
			Win::Message msg (LVM_ENSUREVISIBLE, item, FALSE);
			SendMsg (msg);
		}

		void PostEnsureVisible (int item)
		{
			Win::Message msg (LVM_ENSUREVISIBLE, item, FALSE);
			PostMsg (msg);
		}

		int GetSelectedCount () const
		{
			return ListView_GetSelectedCount (H ());
		}

		bool IsSelected (int item) const
		{
			return (ListView_GetItemState (H (), item, LVIS_SELECTED) & LVIS_SELECTED) != 0;
		}

		int GetFirstSelected () const
		{
			return ListView_GetNextItem (H (), -1, LVNI_SELECTED);
		}

		int GetNextSelected (int iStart) const
		{
			// in a case of one item on the list the call:
			// ListView_GetNextItem (H (), 0, LVNI_BELOW | LVNI_SELECTED);
			// returns 0 if the item is selected. Otherwise it returns -1
			// I change this behaviour so that the call
			// ListView_GetNextItem (H (), 0, LVNI_BELOW | LVNI_SELECTED);
			// always returns -1. Piotr
			if (GetCount () == 1)
				return -1;

			return ListView_GetNextItem (H (), iStart, LVNI_BELOW | LVNI_SELECTED);
		}

		std::string RetrieveItemText (int pos, int subItem = 0) const;
		int GetItemParam (int item, int subItem = 0) const;

		int GetTopIndex () const
		{
			Assert (ListView_GetItemCount (H ()) == _count);
			int top = ListView_GetTopIndex (H ());
			return (top < _count)? top: 0;
		}

		int GetCountPerPage () const
		{
			return ListView_GetCountPerPage (H ());
		}

	protected:
		ListView (Win::Dow::Handle hwndParent, int id)
			: Win::SimpleControl (hwndParent, id),
			  _count (0),
			  _maxItemTextLen (1)
		{}
		ListView (Win::Dow::Handle hwnd)
			: Win::SimpleControl (hwnd),
			  _count (0),
			  _maxItemTextLen (1)
		{}
		ListView ()
			: Win::SimpleControl (0),
			  _count (0),
			  _maxItemTextLen (1)
		{}

	protected:
		int	_count;
		mutable int	_maxItemTextLen;
	};

	inline Win::Style & operator<< (Win::Style & style, Win::ListView::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	inline Win::Style & operator<< (Win::Style & style, Win::ListView::Style::Ex::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Ex::Bits> (bits));
		return style;
	}

	// ListView with columns
	// Base class for other Reports
	class Report: public ListView
	{
	public:
		enum ColAlignment
		{
			Left = LVCFMT_LEFT,
			Center = LVCFMT_CENTER,
			Right = LVCFMT_RIGHT
		};

		class Column: public LVCOLUMN
		{
		public:
			class Mask
			{
			public:
				Mask () : _mask (0) {}
				operator unsigned int () { return _mask; }
				void Format () { _mask |= LVCF_FMT; }
				void Image () { _mask |= LVCF_IMAGE; }
				void Order () { _mask |= LVCF_ORDER; }
				void SubItem () { _mask |= LVCF_SUBITEM; }
				void Text () { _mask |= LVCF_TEXT; }
				void Width () { _mask |= LVCF_WIDTH; }
				void HasImages () { _mask |= LVCFMT_COL_HAS_IMAGES; }
			private:
				unsigned int _mask;
			};
		public:
			Column ();
			Column (ListView & listView, int col, Mask mask);
			void Align (ColAlignment align)
			{
				mask |= LVCF_FMT;
				fmt = align;
			}
			ColAlignment GetAligmment () const
			{
				return static_cast<Win::Report::ColAlignment>(fmt & LVCFMT_JUSTIFYMASK);
			}
			void Width (int width)
			{
				mask |= LVCF_WIDTH;
				cx = width;
			}
			void Title (std::string const & title)
			{
				mask |= LVCF_TEXT;
				pszText = const_cast<char *> (title.c_str ());
			}
			void HasImages ()
			{
				mask |= LVCFMT_COL_HAS_IMAGES;
			}
			void SubItem (int col)
			{
				mask |= LVCF_SUBITEM;
				iSubItem = col;
			}
			void NoImage ()
			{
				fmt &= ~LVCFMT_IMAGE;
			}
			// Note: Use Win::Header to set image
			void Image (int iImg, bool imageOnTheRight)
			{
				fmt |= LVCFMT_IMAGE;
				if (imageOnTheRight)
					fmt |= LVCFMT_BITMAP_ON_RIGHT;
				mask |= (LVCF_FMT | LVCF_IMAGE);
				iImage = iImg;
			}
		};

	public:
		void Init (Win::Dow::Handle win)
		{
			SimpleControl::Reset (win.ToNative ());
			ListView_SetExtendedListViewStyle (H (), LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		}
		void Init (Win::Dow::Handle winParent, int id)
		{
			SimpleControl::Init (winParent, id);
			ListView_SetExtendedListViewStyle (H (), LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		}

		void AddColumn (int width, std::string const & title, ColAlignment align = Left, bool hasImage = false);
		void AddProportionalColumn (int widthPercentage, std::string const & title, ColAlignment align = Left, bool hasImage = false);
		void DeleteColumn (unsigned int iCol);
		Win::Header GetHeader () const 
		{
			return Win::Header ((HWND)SendMsg (LVM_GETHEADER)); 
		}
		void RemoveColumnImage (unsigned int iCol);
		void SetColumnImage (unsigned int iCol, int iImage, bool imageOnTheRight = true);

		void SetColumnText (unsigned int iCol, char const * title, ColAlignment align = Left, bool hasImage = false);
		void ClearAll ();

		unsigned int GetColCount () const
		{
			return _cColumn;
		}

		int GetColumnWidth (int iCol) const
		{
			return ListView_GetColumnWidth (H (), iCol);
		}

		void SetColumnWidth (int iCol, int width)
		{
			ListView_SetColumnWidth (H (), iCol, width);
		}

		void InPlaceEdit (int item)
		{
			ListView_EditLabel (H (), item);
		}

		void AddCheckBoxes ()
		{
			ListView_SetExtendedListViewStyleEx (H (), LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
		}

		void RemoveCheckBoxes ()
		{
			ListView_SetExtendedListViewStyleEx (H (), LVS_EX_CHECKBOXES, 0);
		}

		void Check (int item)
		{
			ListView_SetCheckState (H (), item, TRUE);
		}

		void Uncheck (int item)
		{
			ListView_SetCheckState (H (), item, FALSE);
		}

		bool IsChecked (int item) const
		{
			return ListView_GetCheckState (H (), item) != 0;
		}

		void CheckAll ()
		{
			int count = GetCount ();
			for (int i = 0; i < count; i++)
				Check (i);
		}

		void UncheckAll ()
		{
			int count = GetCount ();
			for (int i = 0; i < count; i++)
				Uncheck (i);
		}

		int GetHitItem (Win::Point const & dropPoint);

	protected:
		Report (Win::Dow::Handle hwndParent, int id)
			: ListView (hwndParent, id),
			  _cColumn (0)
		{}
		Report (Win::Dow::Handle hwnd)
			: ListView (hwnd),
			  _cColumn (0)
		{}
		Report ()
			: _cColumn (0)
		{}
	protected:
		unsigned int	_cColumn;
	};

	// Report based on client callbacks
	// Implement ListViewHandler::OnItemChanged
	// to set Item's text, icon, and overlay
	class ReportCallback: public Report
	{
	public:
		ReportCallback (Win::Dow::Handle hwnd)
			: Report (hwnd)
		{
			_maxItemTextLen = 100;
		}
		ReportCallback ()
		{
			_maxItemTextLen = 100;
		}

		void Reserve (int newItemCount);
		void AddItems (int count);
		void AppendItem ();
	};

	// Report with settable items
	class ReportListing: public Report
	{
	public:
		ReportListing (Win::Dow::Handle hwndParent, int id)
			: Report (hwndParent, id)
		{}
		ReportListing (Win::Dow::Handle hwnd = Win::Dow::Handle ())
			: Report (hwnd)
		{}
		void AddSubItem (char const *itemString, int item, int subItem);
		void AddSubItem (std::string const & itemString, int item, int subItem)
		{
			AddSubItem (itemString.c_str (), item, subItem);
		}
		void InsertItem (int pos, ListView::Item & item);
		void UpdateItem (int pos, ListView::Item & item);
		// these methods return index of the item, -1 if not successful
		int AppendItem (char const * itemString, int itemData = 0);
		int AppendItem (ListView::Item & item);
		int FindItemByName (char const *itemString);
		int FindItemByData (int itemData);
	};

	class ListViewMaker : public Win::ControlMaker
	{
	public:
		// Revisit:
		// add class Style
		// add class ExStyle
		// pass Style to constructor
		// pass ExStyle to Create
		ListViewMaker (Win::Dow::Handle hwndParent, int id)
			: ControlMaker (WC_LISTVIEW, hwndParent, id)
		{
			Style () << ListView::Style::Ex::FullRowSelect
					 << Win::Style::ClipSiblings 
					 << Win::Style::ClipChildren;
		}
		Win::Dow::Handle Create ()
		{
			return ControlMaker::Create ();
		}
	};

	class ReportMaker : public ListViewMaker
	{
	public:
		ReportMaker (Win::Dow::Handle hwndParent, int id)
			: ListViewMaker (hwndParent, id)
		{
			Style () << ListView::Style::Ex::FullRowSelect
					 << ListView::Style::MakeReport
					 << ListView::Style::ShowSelAlways;
		}
	};
}

namespace Keyboard { class Handler; }

namespace Notify
{
	// Subclass ListViewHandler overwriting some of its methods
	// In your controller, overwrite the following method to return your handler
	// Notify::Handler * Win::Controller::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom)
	class ListViewHandler : public Notify::Handler
	{
	public:
		explicit ListViewHandler (unsigned id): Notify::Handler (id) {}
		virtual bool OnDblClick () throw ()
			{ return false; }
		virtual bool OnClick () throw ()
			{ return false; }
		virtual bool OnGetDispInfo (Win::ListView::Request const & request,
									Win::ListView::State const & state,
									Win::ListView::Item & item) throw ()
			{ return false; }
		virtual bool OnItemChanged (Win::ListView::ItemState & state) throw ()
			{ return false; }
		virtual bool OnBeginLabelEdit (long & result) throw ()
			{ return false; }
		virtual bool OnEndLabelEdit (Win::ListView::Item * item, long & result) throw ()
			{ return false; }
		virtual bool OnColumnClick (int col) throw ()
			{ return false; }
		virtual bool OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
			{ return false; }
		virtual Keyboard::Handler * GetKeyboardHandler () throw ()
			{ return 0; }
		virtual void OnBeginDrag (Win::Dow::Handle winFrom, unsigned idFrom, int itemIdx, bool isRightButtonDrag) throw ()
			{}
		virtual void OnCustomDraw (Win::ListView::CustomDraw & customDraw, long & result) throw ()
		{ result = Win::ListView::CustomDraw::DoDefault; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}

#endif
