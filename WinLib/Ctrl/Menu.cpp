//-----------------------------------
// (c) Reliable Software 1998 -- 2004
//-----------------------------------

#include <WinLibBase.h>
#include "Menu.h"


template<>
void Win::Disposal<Menu::Handle>::Dispose (Menu::Handle h) throw ()
{
	::DestroyMenu (h.ToNative ());
}

namespace Menu 
{
	void Refresh (Menu::Handle menu, 
				  Menu::Item const * popup, 
				  Cmd::Vector & cmdVector,
				  bool hideDisabled = false) throw ()
	{
		menu.Clear ();
		bool rememberAddSeparator = false;
		std::string dynamicDisplayName;
		for (int curItem = 0; popup [curItem].kind != END; ++curItem)
		{
			Menu::ItemKind kind = popup [curItem].kind;
			if (kind == CMD)
			{
				// Associate controller method with menu item
				Cmd::Status cmdStatus = cmdVector.Test (popup [curItem].cmdName);
				if (cmdStatus != Cmd::Invisible)
				{
					if (hideDisabled && cmdStatus == Cmd::Disabled)
						continue;

					if (rememberAddSeparator)
					{
						// Add separators only if needed
						if (!menu.IsEmpty ())
							menu.AddSeparator ();
						rememberAddSeparator = false;
					}

					char const * displayName = popup [curItem].displayName;
					int itemId = cmdVector.Cmd2Id (popup [curItem].cmdName);
					menu.AddItem (itemId, displayName);

					switch (cmdStatus)
					{
					case Cmd::Disabled:
						menu.Disable (itemId);
						break;
					case Cmd::Enabled:
						menu.Enable (itemId);
						menu.UnCheck (itemId);
						break;
					case Cmd::Checked:
						menu.Enable (itemId);
						menu.Check (itemId);
						break;
					}				
				}
			}
			else if (kind == POP)
			{
				if (rememberAddSeparator)
				{
					// Add separators only if needed
					if (!menu.IsEmpty ())
						menu.AddSeparator ();
					rememberAddSeparator = false;
				}
				Menu::Popup subPopup;
				Menu::Refresh (subPopup, popup [curItem].popup, cmdVector, hideDisabled);
				menu.AddPopup (subPopup, popup [curItem].displayName);
			}
			else
			{
				Assert (kind == SEPARATOR);			
				rememberAddSeparator = true;
			}
		}
	}
}

Menu::DropDown::DropDown (Menu::Item const * templ, Cmd::Vector & cmdVector)
    : _barItemCnt (0),
	  _template (templ),
	  _cmdVector (cmdVector)
{
    // Count menu bar items
	while (_template [_barItemCnt].kind != END)
		_barItemCnt++;

    // Create menu
    for (int curMenu = 0; curMenu < _barItemCnt; curMenu++)
    {
        Assert (_template [curMenu].kind == Menu::POP);
		Menu::Popup emptyPopup;
		Menu::AutoHandle curPopup (emptyPopup);
		_bar.AddPopup (curPopup, _template [curMenu].displayName);
    }
}

void Menu::DropDown::AttachToWindow (Win::Dow::Handle hwnd)
{
    if (!::SetMenu (hwnd.ToNative (), _bar.ToNative ()))
        throw Win::Exception ("Internal error: Cannot attach menu.");
}

void Menu::DropDown::RefreshPopup (Menu::Handle menu, int itemNo, bool hideDisabled) throw ()
{	
	if (itemNo < 0 || itemNo >= _barItemCnt)
		return;

	Menu::Item const * popup = _template [itemNo].popup;

	Menu::Refresh (menu, popup, _cmdVector, hideDisabled);
}

void Menu::Handle::Clear ()
{
	int countItem = ::GetMenuItemCount (ToNative ());
	for (int k = countItem - 1; k >= 0; --k)
		::DeleteMenu (ToNative (), k, MF_BYPOSITION);
}

int Menu::DropDown::GetBarItem (std::string const & name) const
{
	for (int i = 0; _template [i].kind != END; ++i)
	{
		if (name == _template [i].cmdName)
			return i;
	}
	return -1;
}

Menu::Item const * Menu::DropDown::GetPopupTemplate (std::string const & menuName) const
{
	int index = GetBarItem (menuName);
	Assert (index >= 0);
	Assert (_template [index].kind == Menu::POP);
	return _template [index].popup;
}

void Menu::Tracker::TrackMenu (Win::Dow::Handle winOwner, int x, int y)
{
	if (::TrackPopupMenuEx (H (),
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		x, y,
		winOwner.ToNative (),
		0) == FALSE)
	{
		//				revisit:  different Windows platforms return different errors
		//					e.g. Win95 appears to return 0,
		//						 Win98 & WinME return ERROR_INVALID_PARAMETER,
		//						 undetermined what NT 4 and Win 2K return
		//				if (::GetLastError () != ERROR_POPUP_ALREADY_ACTIVE)
		//					Assert (!"Popup menu tracking failed");
		::SetLastError (0);
	}
}

void Menu::Context::Refresh () throw ()
{
	Menu::Refresh (*this, _template, _cmdVector, true); // hide disabled commands
}
