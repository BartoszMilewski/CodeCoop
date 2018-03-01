//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "ListView.h"
#include <Win/Utility.h>
#include <Win/Keyboard.h>

using namespace Win;
using namespace Notify;

void ListView::ItemManipulator::Init (int pos, char const * text, int data)
{
	mask = LVIF_TEXT | LVIF_PARAM; 
	iItem = pos; 
	pszText = const_cast<char *> (text);
	lParam = data;    // 32-bit value to associate with item 
	cchTextMax = strlen (text); 
}

std::string ListView::RetrieveItemText (int pos, int subItem) const
{
	Assert (_maxItemTextLen != 0);
	// Start with assumption that hint is invalid
	size_t bufSize = _maxItemTextLen + 2;	// +1 for '\0'; +1 to detect invalid hint
	std::vector<char> buf;
	buf.resize (bufSize);
	ItemManipulator item;
	item.PrepareForRead (pos, &buf [0], buf.size (), subItem);
	// charsCopied doesn't include terminating '\0' added by list view control
	size_t charsCopied = SendMsg (LVM_GETITEMTEXT, pos, reinterpret_cast<LPARAM>(&item));
	if (charsCopied == bufSize - 1)
	{
		// List view contains item with text longer then _maxItemTextLen
		do
		{
			bufSize = 2 * bufSize;
			buf.resize (bufSize);
			item.PrepareForRead (pos, &buf [0], buf.size (), subItem);
			charsCopied = SendMsg (LVM_GETITEMTEXT, pos, reinterpret_cast<LPARAM>(&item));
		} while (charsCopied == bufSize - 1);
		// Update hint
		_maxItemTextLen = charsCopied;
	}
	std::string result;
	result.assign (&buf [0]);
	return result;
}

int ListView::GetItemParam (int pos, int subItem) const
{
    ItemManipulator item;
	item.PrepareForRead (pos, subItem);
    ListView_GetItem (ToNative (), &item);
    return item.lParam;
}

Report::Column::Column ()
{
	memset (this, 0, sizeof (LVCOLUMN));
}

Report::Column::Column (Win::ListView & listView, int col, Report::Column::Mask what)
{
	memset (this, 0, sizeof (LVCOLUMN));
	mask = what;
	if (ListView_GetColumn (listView.ToNative (), col, this) == FALSE)
		throw Win::Exception ("Internal error: Cannot get column data.");
}

void Report::AddColumn (int width, std::string const & title, ColAlignment align, bool hasImage)
{
	Report::Column column;
	column.Width (width);
	column.Align (align);
	if (!title.empty ())
		column.Title (title);
	column.SubItem (_cColumn);
	if (hasImage)
		column.HasImages ();
    if (ListView_InsertColumn (H (), _cColumn, &column) == -1)
        throw Win::Exception ("Internal error: Cannot insert column.");
    _cColumn++;
}

void Report::AddProportionalColumn (int widthPercentage, std::string const & title, ColAlignment align, bool hasImage)
{
    ClientRect rect (H ());
    // Revisit: check if total number of items is less then items/per page
    // and if this is not the case substract from list view client rectangle
    // width the system width of vertical scroll bar
	Report::Column column;
	column.Width ((widthPercentage * rect.Width ()) / 100);
	column.Align (align);
	if (!title.empty ())
		column.Title (title);
	column.SubItem (_cColumn);
	if (hasImage)
		column.HasImages ();
    if (ListView_InsertColumn (H (), _cColumn, &column) == -1)
        throw Win::Exception ("Internal error: Cannot insert column.");
    _cColumn++;
}

void Report::DeleteColumn (unsigned int iCol)
{
    Assert (iCol < _cColumn);
    if (ListView_DeleteColumn (H (), iCol) == FALSE)
        throw Win::Exception ("Internal error: Cannot delete column.");
    _cColumn--;
}

void Report::RemoveColumnImage (unsigned int iCol)
{
    Assert (iCol < _cColumn);
	Win::Header header = GetHeader ();
	header.RemoveColumnImage (iCol);
#if 0
	Report::Column::Mask mask;
	mask.Format ();
	Report::Column column (*this, iCol, mask);
	column.NoImage ();
	if (ListView_SetColumn (H (), iCol, &column) == FALSE)
		throw Win::Exception ("Internal error: Cannot clear column header image.");
#endif
}

void Report::SetColumnImage (unsigned int iCol, int iImage, bool imageOnTheRight)
{
    Assert (iCol < _cColumn);
	Win::Header header = GetHeader ();
	header.SetColumnImage (iCol, iImage);
#if 0
	// This is inferior: it doesn't take into account transparency
	Report::Column::Mask mask;
	mask.Format ();
	Report::Column column (*this, iCol, mask);

	column.Image (iImage, imageOnTheRight);
	if (ListView_SetColumn (H (), iCol, &column) == FALSE)
		throw Win::Exception ("Internal error: Cannot set column header image.");
#endif
}

void Report::SetColumnText (unsigned int iCol, char const * title, ColAlignment align, bool hasImage)
{
    Assert (iCol < _cColumn);
	Win::Header header = GetHeader ();
	header.SetColumnText (iCol, title);
#if 0
	Report::Column column;
	column.Align (align);
	if (title != 0 && title [0] != 0)
		column.Title (title);
	column.SubItem (iCol);
	if (hasImage)
		column.HasImages ();
	if (ListView_SetColumn (H (), iCol, &column) == FALSE)
		throw Win::Exception ("Internal error: Cannot set column header text.");
#endif
}

void Report::ClearAll ()
{
	ClearRows ();
    for (int i = _cColumn - 1; i >= 0; i--)
    {
        DeleteColumn (i);
    }
    _count = 0;
}

int Report::GetHitItem (Win::Point const & dropPoint)
{
	LVHITTESTINFO hitInfo;
	::ZeroMemory (&hitInfo, sizeof (LVHITTESTINFO));
	hitInfo.pt = dropPoint;
	return ListView_HitTest (H (), &hitInfo);
}

void ReportCallback::Reserve (int newItemCount)
{
	if (_count == newItemCount)
		return;

	if (newItemCount > _count)
	{
		while (newItemCount > _count)
			AppendItem ();
	}
	else
	{
		ClearRows ();
		AddItems (newItemCount);
	}

	Assert (_count == newItemCount);
	Assert (GetCount () == _count);
}

void ReportCallback::AddItems (int count)
{
	ItemManipulator item;
    item.MakeCallback (_maxItemTextLen);

    _count = count;
    ListView_SetItemCount (H (), count);
    for (int i = 0; i < count; i++)
    {
        item.SetPos (i);
        ListView_InsertItem(H (), &item);
    }
}

void ReportCallback::AppendItem ()
{
	ItemManipulator item;
    item.MakeCallback (_maxItemTextLen);
    item.SetPos (_count);
    _count++;
    ListView_InsertItem(H (), &item);
}

int ReportListing::AppendItem (char const * itemString, int itemData)
{
	ItemManipulator item;
    item.Init (_count, itemString, itemData);
    _count++;
	if (_maxItemTextLen < item.GetTextLen ())
		_maxItemTextLen = item.GetTextLen ();
    return ListView_InsertItem (H (), &item);
}

int ReportListing::AppendItem (Item & item)
{
	item.SetPos (_count);
    _count++;
	if (_maxItemTextLen < item.GetTextLen ())
		_maxItemTextLen = item.GetTextLen ();
    return ListView_InsertItem (H (), &item);
}

void ReportListing::InsertItem (int pos, Item & item)
{
	item.SetPos (pos);
    _count++;
	if (_maxItemTextLen < item.GetTextLen ())
		_maxItemTextLen = item.GetTextLen ();
    ListView_InsertItem (H (), &item);
}

void ReportListing::UpdateItem (int pos, Item & item)
{
	item.SetPos (pos);
	if (_maxItemTextLen < item.GetTextLen ())
		_maxItemTextLen = item.GetTextLen ();
    ListView_SetItem (H (), &item);
}

void ReportListing::AddSubItem (char const *itemString, int item, int subItem)
{
	int textLen = strlen (itemString);
	if (_maxItemTextLen < textLen)
		_maxItemTextLen = textLen;
	ListView_SetItemText(H (), item, subItem, const_cast<char *> (itemString));
}

int ReportListing::FindItemByName (char const * itemString)
{
    LV_FINDINFO findInfo;

    findInfo.flags = LVFI_STRING; 
    findInfo.psz = itemString; 
    return ListView_FindItem(H (), -1, &findInfo);
}

int ReportListing::FindItemByData (int itemData)
{
    LV_FINDINFO findInfo;

    findInfo.flags = LVFI_PARAM; 
    findInfo.lParam = itemData; 
    return ListView_FindItem(H (), -1, &findInfo);
}

bool ListViewHandler::OnNotify (NMHDR * hdr, long & result)
{
	// hdr->code
	// hdr->idFrom;
	// hdr->hwndFrom;
	switch (hdr->code)
	{
	case NM_DBLCLK:
		return OnDblClick ();
	case NM_CLICK:
		return OnClick ();
	case LVN_GETDISPINFO:
		{
			LV_DISPINFO * dispInfo = reinterpret_cast<LV_DISPINFO *>(hdr);
			ListView::Item * item = reinterpret_cast<ListView::Item *>(&dispInfo->item);
			ListView::Request request (*item);
			ListView::State state (*item);
			item->Unmask ();
			return OnGetDispInfo (request, state, *item);
		}
	case LVN_ITEMCHANGED:
		{
			Win::ListView::ItemState state (hdr);
			return OnItemChanged (state);
		}
	case LVN_BEGINLABELEDIT:
		return OnBeginLabelEdit (result);
	case LVN_ENDLABELEDIT:
		{
			LV_DISPINFO * dispInfo = reinterpret_cast<LV_DISPINFO *>(hdr);
			ListView::Item * item = reinterpret_cast<ListView::Item *>(&dispInfo->item);
			return OnEndLabelEdit (item, result);
		}
	case LVN_COLUMNCLICK:
		{
			NMLISTVIEW * lview = reinterpret_cast<NM_LISTVIEW *>(hdr);
			return OnColumnClick (lview->iSubItem);
		}
	case NM_SETFOCUS :
		return OnSetFocus (hdr->hwndFrom, hdr->idFrom);
	case LVN_KEYDOWN:
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
	case LVN_BEGINDRAG:
		{
			NMLISTVIEW * lview = reinterpret_cast<NM_LISTVIEW *>(hdr);
			OnBeginDrag (lview->hdr.hwndFrom, lview->hdr.idFrom, lview->iItem, false);
			return true;
		}
		break;
	case LVN_BEGINRDRAG:
		{
			NMLISTVIEW * lview = reinterpret_cast<NM_LISTVIEW *>(hdr);
			OnBeginDrag (lview->hdr.hwndFrom, lview->hdr.idFrom, lview->iItem, true);
			return true;
		}
		break;
	case NM_CUSTOMDRAW:
		{
			Win::ListView::CustomDraw * customDraw = reinterpret_cast<Win::ListView::CustomDraw *>(hdr);
			OnCustomDraw (*customDraw, result);
			return true;
		}
		break;
	}
	return false;
}

