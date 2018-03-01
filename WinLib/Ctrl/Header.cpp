//------------------------------------
//  (c) Reliable Software, 2006 - 2007
//------------------------------------

#include <WinLibBase.h>
#include "Header.h"

Win::Header::Item::Item (Win::Header & header, int colIdx, Win::Header::Item::Mask what)
{
	::ZeroMemory (this, sizeof (HDITEM));
	mask = what;
	if (Header_GetItem (header.ToNative (), colIdx, this) == FALSE)
		throw Win::Exception ("Internal error: Cannot get header item data.");
}

int Win::Header::AppendItem (char const * title, int width)
{
	Win::Header::Item item;
	item.SetText (title);
	item.SetWidth (width);
	return Header_InsertItem (H (), Header_GetItemCount(H ()), &item);
}

void Win::Header::SetItemWidth (int col, int width)
{
	Win::Header::Item item;
	item.SetWidth (width);
	Header_SetItem (H (), col, &item);
}

void Win::Header::SetColumnImage (unsigned int iCol, int iImage)
{
	Win::Header::Item::Mask mask;
	mask.Format ();
	Win::Header::Item item (*this, iCol, mask);
	item.SetImage (iImage);
	Header_SetItem (H (), iCol, &item);
}

void Win::Header::RemoveColumnImage (unsigned int iCol)
{
	Win::Header::Item::Mask mask;
	mask.Format ();
	Win::Header::Item item (*this, iCol, mask);
	item.ResetImage ();
	Header_SetItem (H (), iCol, &item);
}

void Win::Header::SetColumnText (unsigned int iCol, char const * title)
{
	Win::Header::Item::Mask mask;
	mask.Format ();
	Win::Header::Item item (*this, iCol, mask);
	item.SetText (title);
	Header_SetItem (H (), iCol, &item);
}

