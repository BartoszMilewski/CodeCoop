#if !defined (TREE_H)
#define TREE_H
//----------------------------------
// (c) Reliable Software 2002 - 2008
//----------------------------------

#include <Ctrl/Controls.h>
#include <Win/Message.h>
#include <Win/Notification.h>

namespace Tree
{
	class Item;

	typedef Win::Handle<HTREEITEM> NodeHandle;

	class Node: public TVINSERTSTRUCT
	{
		// UINT      mask;
		// HTREEITEM hItem;
		// UINT      state;
		// UINT      stateMask;
		// LPTSTR    pszText;
		// int       cchTextMax;
		// int       iImage;
		// int       iSelectedImage;
		// int       cChildren;
		// LPARAM    lParam;
	public:
		Node (NodeHandle parent = NodeHandle ())
		{
			hParent = parent.ToNative ();
			hInsertAfter = 0;
			memset (&item, 0, sizeof (TVITEM));
			item.mask |= TVIF_STATE; // state and stateMask are valid
			item.stateMask = 0xffffffff;
		}
		void ToInsertAfter (NodeHandle node)
		{
			hInsertAfter = node.ToNative ();
		}
		void SetText (char const * text)
		{
			item.mask |= TVIF_TEXT; 
			item.pszText = const_cast<char *>(text);
			item.cchTextMax = strlen (text); 
		}
		void SetTextCallback ()
		{
			item.mask |= TVIF_TEXT; 
			item.pszText = LPSTR_TEXTCALLBACK;
		}
		void SetIcon (int iconId, int iconSel)
		{
			item.mask |= (TVIF_IMAGE | TVIF_SELECTEDIMAGE);
			item.iImage = iconId;
			item.iSelectedImage = iconSel;
		}
		void SetParent (NodeHandle parent)
		{
			hParent = parent.ToNative ();
		}
		void SetHasChildren (bool does)
		{
			item.mask |= TVIF_CHILDREN;
			item.cChildren = does? 1: 0;
		}
		void SetChildCountCallback ()
		{
			item.mask |= TVIF_CHILDREN;
			item.cChildren = I_CHILDRENCALLBACK;
		}
		template<class T>
		void SetUserPtr (T * p)
		{
			item.mask |= TVIF_PARAM;
			item.lParam = reinterpret_cast<LPARAM> (p);
		}
	};

	class View: public Win::SimpleControl
	{
	public:
		class Item: public	TVITEM
		{
			// UINT      mask;
			// HTREEITEM hItem;
			// UINT      state;
			// UINT      stateMask;
			// LPTSTR    pszText;
			// int       cchTextMax;
			// int       iImage;
			// int       iSelectedImage;
			// int       cChildren;
			// LPARAM    lParam;
		public:
			Item ()
			{
				::ZeroMemory (this, sizeof (Item));
			}
			Item (NodeHandle node)
			{
				::ZeroMemory (this, sizeof (Item));
				hItem = node.ToNative ();
			}
			void Unmask ()
			{
				mask = 0;
				stateMask = 0;
				state = 0;
			}
			NodeHandle GetNode () const { return hItem; }
			char * GetBuf () { return pszText; }
			unsigned GetTextLen () const { return cchTextMax; }
			unsigned GetChildCount () const { return cChildren; }
			template<class T>
			T * GetUserPtr ()
			{
				return reinterpret_cast<T *> (lParam);
			}
			void SetChildCount (unsigned count)
			{
				mask |= TVIF_CHILDREN;
				cChildren = count;
			}
			void SetIcon (int img, int imgSelected)
			{
				mask |= TVIF_IMAGE;
				iImage = img;
				SetSelIcon (imgSelected);
			}
			void SetSelIcon (int img)
			{
				mask |= TVIF_SELECTEDIMAGE;
				iSelectedImage = img;
			}
			void SetText (bool remember = false) 
			{
				mask |= TVIF_TEXT;
				if (remember)
					mask |= TVIF_DI_SETITEM;
			}
		};

		class Request
		{
		public:
			Request (Item const & item)
				: _mask (item.mask)
			{}
			bool IsText () const { return (_mask & TVIF_TEXT) != 0; }
			bool IsIcon () const { return (_mask & TVIF_IMAGE) != 0; }
			bool IsParam () const { return (_mask & TVIF_PARAM) != 0; }
			bool IsState () const { return (_mask & TVIF_STATE) != 0; }
			bool IsChildCount () const { return (_mask & TVIF_CHILDREN) != 0; }
		private:
			unsigned _mask;
		};

		class State
		{
		public:
			State (Item const & item)
				: _mask (item.stateMask),
				  _state (item.state)
			{}
		private:
			unsigned _mask;
			unsigned _state;
		};

	public:
		class Style: public Win::Style
		{
		public:
			enum Bits
			{
				HasLines = TVS_HASLINES,
				LinesAtRoot = TVS_LINESATROOT,
				HasButtons = TVS_HASBUTTONS,
				EditLabels = TVS_EDITLABELS,
				ShowSelAlways = TVS_SHOWSELALWAYS,
				CheckBoxes = TVS_CHECKBOXES,
				TrackSelect = TVS_TRACKSELECT
			};
		};

	public:
		// View Methods
		View (Win::Dow::Handle win = 0) : Win::SimpleControl (win) {}
		Tree::NodeHandle HitTest (Win::Point pt) const;
		void AttachImageList (ImageList::Handle imageList);
		Tree::NodeHandle InsertRoot (Tree::Node & item);
		Tree::NodeHandle AppendChild (Tree::Node & item);
		void Select (Tree::NodeHandle node);
		void SetHilite (Tree::NodeHandle node);
		void RemoveHilite (Tree::NodeHandle node);
		Tree::NodeHandle GetSelection ();
		void Expand (Tree::NodeHandle parent);
		void SetChildless (Tree::NodeHandle node);
		void Collapse (Tree::NodeHandle parent, bool forget);
		void ClearAll ();
		unsigned int GetItemCount () const;
	};

	inline Win::Style & operator<<(Win::Style & style, Tree::View::Style::Bits bits)
	{
		style.OrIn (static_cast<Win::Style::Bits> (bits));
		return style;
	}

	class Maker: public Win::ControlMaker
	{
	public:
		Maker (Win::Dow::Handle parentWin, int id);
		Win::Dow::Handle Create ()
		{
			return ControlMaker::Create ();
		}
		Tree::View::Style & Style () { return static_cast<Tree::View::Style &> (_style); }
	};
}

namespace Tree
{
	namespace Notify
	{
		enum Action
		{
			ByKeyboard = TVC_BYKEYBOARD,
			ByMouse = TVC_BYMOUSE,
			Unknown = TVC_UNKNOWN
		};
	}
}

namespace Keyboard { class Handler; }

namespace Notify
{
	// Subclass TreeViewHandler overwriting some of its methods
	// In your controller, overwrite the following method to return your handler
	// Notify::Handler * Win::Controller::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom)
	class TreeViewHandler : public Notify::Handler
	{
	public:
		explicit TreeViewHandler (unsigned id) : Notify::Handler (id) {}
		virtual bool OnGetDispInfo (Tree::View::Request const & request,
				Tree::View::State const & state,
				Tree::View::Item & item) throw ()
			{ return false; }
		virtual bool OnItemExpanding (Tree::View::Item & item,
									Tree::Notify::Action action,
									bool & allow) throw ()
			{ allow = false; return false; }
		virtual bool OnItemExpanded (Tree::View::Item & item,
									Tree::Notify::Action action) throw ()
			{ return false; }
		virtual bool OnItemCollapsing (Tree::View::Item & item,
									Tree::Notify::Action action,
									bool & allow) throw ()
			{ allow = false; return false; }
		virtual bool OnItemCollapsed (Tree::View::Item & item,
									Tree::Notify::Action action) throw ()
			{ return false; }
		virtual bool OnSelChanging (Tree::View::Item & itemOld, 
									Tree::View::Item & itemNew,
									Tree::Notify::Action action) throw ()
			{ return false; }
		virtual bool OnSelChanged (Tree::View::Item & itemOld, 
									Tree::View::Item & itemNew,
									Tree::Notify::Action action) throw ()
			{ return false; }
		virtual bool OnSetFocus (Win::Dow::Handle winFrom, unsigned idFrom)
			{ return false; }
		virtual bool OnClick (Win::Point pt) throw ()
			{ return false; }
		virtual bool OnRClick (Win::Point pt) throw ()
			{ return false; }
		virtual bool OnDblClick (Win::Point pt) throw ()
			{ return false; }
		virtual Keyboard::Handler * GetKeyboardHandler () throw ()
			{ return 0; }

	protected:
		bool OnNotify (NMHDR * hdr, long & result);
	};
}

#endif
