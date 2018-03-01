//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <WinLibBase.h>
#include "StatusBar.h"

std::string Win::StatusBar::GetString (int idx) const
{
	std::vector<char> text;
	std::string result;
    int len = GetTextLen (idx);
    if (len > 0)
    {
		text.resize (len + 1);
		SendMsg (SB_GETTEXT, idx, reinterpret_cast<LPARAM>(&text [0]));
		result.assign (&text [0]);
    }
	return result;
}

void Win::StatusBar::ReSize (int left, int top, int width, int height)
{
	Move (left, top, width, height);
	int pos = 0;
	for (unsigned int i = 0; i < _partsPerCent.size (); i++)
	{
		pos += _partsPerCent [i] * width / 100;
		_parts[i] = pos;
	}
	SendMsg (SB_SETPARTS, _parts.size (), reinterpret_cast<LPARAM>(&_parts.front ()));
}

Win::StatusBarMaker::StatusBarMaker (Win::Dow::Handle winParent, int id)
	: ControlMaker ("", winParent, id)
{
	Style () << StatusBar::Style::Bottom 
				<< StatusBar::Style::SizeGrip 
				<< Win::Style::ClipChildren 
				<< Win::Style::ClipSiblings;
}

Win::Dow::Handle Win::StatusBarMaker::Create ()
{
	Win::Dow::Handle h = ::CreateStatusWindow (_style.GetStyleBits (),
												"", 
												_hWndParent.ToNative (), 
												reinterpret_cast<unsigned int>(_hMenu));
	if (h.IsNull ())
		throw Win::Exception ("Internal error: Couldn't create status bar");
	return h;
}
