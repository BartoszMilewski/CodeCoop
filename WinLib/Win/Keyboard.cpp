//----------------------------------------------------
// Keyboard.cpp
// (c) Reliable Software 2000
//
//----------------------------------------------------

#include <WinLibBase.h>
#include "Keyboard.h"

using namespace Keyboard;

bool Handler::OnKeyDown (int vKey, int flags)
{
	if (vKey <= VKey::Help)
	{
		// Navigation keys
		switch (vKey)
		{
		case VKey::Prior: 
			return OnPageUp ();
		case VKey::Next: 
			return OnPageDown ();
		case VKey::Up:
			return OnUp (); 
		case VKey::Down:
			return OnDown (); 
		case VKey::Left: 
			return OnLeft (); 
		case VKey::Right:
			return OnRight ();
		case VKey::Home:
			return OnHome ();
		case VKey::End: 
			return OnEnd (); 
		case VKey::Return: 
			return OnReturn ();
		case VKey::Delete:
			return OnDelete (); 
		case VKey::BackSpace:
			return OnBackSpace ();
		case VKey::Escape: 
			return OnEscape (); 
		case VKey::Tab:
			return OnTab ();
		default: 
			return false;
		}
	}
	else if (vKey <= VKey::Z)
	{
		return OnCharKey (vKey);
	}
	else if (vKey <= VKey::Divide)
	{
		return OnNumpad (vKey);
	}
	else if (vKey <= VKey::F24)
	{
		return OnFunctionKey (vKey - VKey::F1 + 1);
	}
	return false;
}