//----------------------------------------------------
// (c) Reliable Software 1998, 99, 2000
//----------------------------------------------------

#include <WinLibBase.h>
#include <Ctrl/Tabs.h>

using namespace Win;
using namespace Notify;

int Tab::Handle::GetCount () const
{
	return SendMsg (TCM_GETITEMCOUNT);
}

int Tab::Handle::AddTab (int itemIdx, char const * caption, long param)
{
    TC_ITEM tie;

    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1;
    tie.pszText = const_cast<char *>(caption);
	if (param >= 0)
	{
		tie.mask |= TCIF_PARAM;
		tie.lParam = param;
	}
    int index = SendMsg (TCM_INSERTITEM, itemIdx, (LPARAM) &tie);
    if (index == -1)
        throw Win::Exception ("Internal error: Cannot add tab item to tab control.");
	return index;
}

int Tab::Handle::AddTab (int itemIdx, Tab::Item const & tabItem)
{
    int index = SendMsg (TCM_INSERTITEM, itemIdx, (LPARAM) &tabItem);
    if (index == -1)
        throw Win::Exception ("Cannot add tab item to tab control.");
	return index;
}

void Tab::Handle::DeleteTab (int itemIdx)
{
	if (SendMsg (TCM_DELETEITEM, itemIdx) != TRUE)
		throw Win::Exception ("Cannot delete tab item");
}

long Tab::Handle::GetParam (int itemIdx) const
{
	TC_ITEM tie;
	tie.mask = TCIF_PARAM;
	if (!GetItem (itemIdx, &tie))
		throw Win::Exception ("Internal error: Cannot get tab item parameter");
	return tie.lParam;
}

void Tab::Handle::SetImage (int itemIdx, int imageIdx)
{
    TC_ITEM tie;

    tie.mask = TCIF_IMAGE; 
    tie.iImage = imageIdx;
    if (!SetItem (itemIdx, &tie))
        throw Win::Exception ("Internal error: Cannot add tab image.");
}

void Tab::Handle::RemoveImage (int itemIdx)
{
    TC_ITEM tie;

    tie.mask = TCIF_IMAGE; 
    tie.iImage = -1;
    if (!SetItem (itemIdx, &tie))
        throw Win::Exception ("Internal error: Cannot remove tab image.");
}

void Tab::Handle::GetDisplayArea (Win::Rect & bigRectangle)
{
	SendMsg (TCM_ADJUSTRECT, (WPARAM) FALSE, (LPARAM) &bigRectangle);
}

void Tab::Handle::GetWindowRect (Win::Rect & bigRectangle)
{
	SendMsg (TCM_ADJUSTRECT, (WPARAM) TRUE, (LPARAM) &bigRectangle);
}

bool TabHandler::OnNotify (NMHDR * hdr, long & result)
{
	// hdr->code
	// hdr->idFrom;
	// hdr->hwndFrom;
	switch (hdr->code)
	{
	case TCN_SELCHANGE:
		return OnSelChange ();
	}
	return false;
}
