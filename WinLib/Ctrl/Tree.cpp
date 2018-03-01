//----------------------------------
// (c) Reliable Software 2002 - 2008
//----------------------------------

#include <WinLibBase.h>

#include "Tree.h"
#include <Graph/ImageList.h>
#include <Win/Keyboard.h>

namespace Notify
{
	bool TreeViewHandler::OnNotify (NMHDR * hdr, long & result)
	{
		NMTREEVIEW * notifyInfo = reinterpret_cast<NMTREEVIEW *>(hdr);
		Tree::View::Item * itemOld = reinterpret_cast<Tree::View::Item *>(&notifyInfo->itemOld);
		Tree::View::Item * itemNew = reinterpret_cast<Tree::View::Item *>(&notifyInfo->itemNew);
		Tree::Notify::Action action = static_cast<Tree::Notify::Action> (notifyInfo->action);
		switch (hdr->code)
		{
		case TVN_GETDISPINFO:
			{
				NMTVDISPINFO * dispInfo = reinterpret_cast<NMTVDISPINFO *> (hdr);
				Tree::View::Item * item = reinterpret_cast<Tree::View::Item *>(&dispInfo->item);
				// make copies
				Tree::View::Request request (*item);
				Tree::View::State state (*item);
				// clear original
				item->Unmask ();
				return OnGetDispInfo (request, state, *item);
			}
			break;
		case TVN_ITEMEXPANDING:
			{
				bool ok = false;
				bool allow = false;
				if (notifyInfo->action == TVE_EXPAND)
					ok = OnItemExpanding (*itemNew, action, allow);
				else if (notifyInfo->action == TVE_COLLAPSE)
					ok = OnItemCollapsing (*itemNew, action, allow);
				result = allow? FALSE: TRUE;
				return ok;
			}
			break;
		case TVN_ITEMEXPANDED:
			if (notifyInfo->action == TVE_EXPAND)
				return OnItemExpanded (*itemNew, action);
			else if (notifyInfo->action == TVE_COLLAPSE)
				return OnItemCollapsed (*itemNew, action);
			break;
		case TVN_SELCHANGING:
			return OnSelChanging (*itemOld, *itemNew, action);
			break;
		case TVN_SELCHANGED:
			return OnSelChanged (*itemOld, *itemNew, action);
			break;
		case NM_SETFOCUS :
			return OnSetFocus (hdr->hwndFrom, hdr->idFrom);
			break;
		case TVN_KEYDOWN:
			{
				NMTVKEYDOWN * keyDown = reinterpret_cast<NMTVKEYDOWN *> (hdr);
				Keyboard::Handler * pHandler = GetKeyboardHandler ();
				if (pHandler != 0)
				{
					if (pHandler->OnKeyDown (keyDown->wVKey, keyDown->flags))
						return true;
				}
			}
			break;
		case NM_RCLICK:
			{
				Win::MsgPosPoint pt;
				return OnRClick (pt);
			}
			break;
		case NM_CLICK:
			{
				Win::MsgPosPoint pt;
				return OnClick (pt);
			}
			break;
		case NM_DBLCLK:
			{
				Win::MsgPosPoint pt;
				return OnDblClick (pt);
			}
			break;
		case TVN_BEGINDRAG:
		case TVN_BEGINRDRAGA:
		case TVN_DELETEITEMA:
		case TVN_BEGINLABELEDITA:
		case TVN_ENDLABELEDITA:
		case TVN_GETINFOTIPA:
		case TVN_SINGLEEXPAND:
		case TVN_SETDISPINFO:
			break;
		case NM_CUSTOMDRAW:
		case NM_SETCURSOR:
			break;
		case NM_KILLFOCUS:
			return true;
			break;
		case NM_CHAR:
			break;
		default:
			// codes are negative
			int nmCode =  0 - hdr->code;
			if (hdr->code >= NM_FIRST && hdr->code <= NM_LAST)
			{
				return false;
			}
			return false;
		}
		return false;
	}
}

namespace Tree
{
	Maker::Maker (Win::Dow::Handle parentWin, int id)
		: Win::ControlMaker (WC_TREEVIEW, parentWin, id)
	{
		Win::CommonControlsRegistry::Instance()->Add(Win::CommonControlsRegistry::TREEVIEW);
	}

	Tree::NodeHandle View::HitTest (Win::Point pt) const
	{
		TVHITTESTINFO hitInfo;
		hitInfo.pt.x = pt.x;
		hitInfo.pt.y = pt.y;

		TreeView_HitTest (H (), &hitInfo);
		return Tree::NodeHandle (hitInfo.hItem);
	}


	void View::AttachImageList (ImageList::Handle imageList)
	{
		Win::Message msg (TVM_SETIMAGELIST);
		msg.SetWParam (TVSIL_NORMAL);
		msg.SetLParam (reinterpret_cast<long> (imageList.ToNative ()));
		SendMsg (msg);
	}
	
	NodeHandle View::InsertRoot (Tree::Node & item)
	{
		item.ToInsertAfter (TVI_ROOT);
		Win::Message msg (TVM_INSERTITEM);
		msg.SetLParam (&item);
		SendMsg (msg);
		return reinterpret_cast<HTREEITEM> (msg.GetResult ());
	}

	NodeHandle View::AppendChild (Tree::Node & item)
	{
		item.ToInsertAfter (TVI_LAST);
		Win::Message msg (TVM_INSERTITEM);
		msg.SetLParam (&item);
		SendMsg (msg);
		if (msg.GetResult () == 0)
			throw Win::Exception ("Cannot append tree node");
		return reinterpret_cast<HTREEITEM> (msg.GetResult ());
	}

	void View::Select (Tree::NodeHandle node)
	{
		Win::Message msg (TVM_SELECTITEM, TVGN_CARET);
		msg.SetLParam (node.ToNative ());
		SendMsg (msg);
		if (msg.GetResult () == 0)
			throw Win::Exception ("Cannot select item");
	}

	void View::SetHilite (Tree::NodeHandle node)
	{
		TreeView_SelectDropTarget (H (),node.ToNative ()); 
	}

	void View::RemoveHilite (Tree::NodeHandle node)
	{
		TreeView_SelectDropTarget (H (), 0); 
	}

	Tree::NodeHandle View::GetSelection ()
	{
		return Tree::NodeHandle (
			TreeView_GetSelection (this->ToNative ()));
	}

	void View::Expand (Tree::NodeHandle parent)
	{
		Win::Message msg (TVM_EXPAND, TVE_EXPAND);
		msg.SetLParam (parent.ToNative ());
		SendMsg (msg);
		if (msg.GetResult () == 0)
			throw Win::Exception ("Cannot expand tree");
	}

	void View::Collapse (Tree::NodeHandle parent, bool forget)
	{
		unsigned flag = TVE_COLLAPSE;
		if (forget)
			flag |= TVE_COLLAPSERESET;
		Win::Message msg (TVM_EXPAND, flag);
		msg.SetLParam (parent.ToNative ());
		SendMsg (msg);
		//if (msg.GetResult () == 0)
		//	throw Win::Exception ("Cannot collapse tree");
	}

	void View::ClearAll ()
	{
		Win::Message msg (TVM_DELETEITEM);
		msg.SetLParam (TVI_ROOT);
		SendMsg (msg);
		if (msg.GetResult () == 0)
			throw Win::Exception ("Cannot clear tree");
	}

	void View::SetChildless (Tree::NodeHandle node)
	{
		Item item (node);
		item.SetChildCount (0);
		Win::Message msg (TVM_SETITEM);
		msg.SetLParam (&item);
		SendMsg (msg);
	}

	unsigned int View::GetItemCount () const
	{
		Win::Message msg (TVM_GETCOUNT);
		SendMsg (msg);
		 return msg.GetResult ();
	}
}
