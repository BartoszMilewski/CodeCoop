//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include <WinLibBase.h>
#include "FontSelector.h"

#include <Graph/Canvas.h>

using namespace Font;

Selector::Selector ()
{
	memset (this, 0, sizeof (Selector));
	_fontSelector.lStructSize = sizeof (CHOOSEFONT);
	_fontSelector.lpLogFont = reinterpret_cast<LOGFONT *>(&_fontMaker);
	// Under NT this will return default system wide keyboard layout
	// Under 95 & 98 this will return current thread keyboard layout
	_keyboard.layout = ::GetKeyboardLayout (0);
	_keyboard.charSet = Font::CharSet::Default;
}

void Selector::LimitSizeSelection (int min, int max)
{
	_fontSelector.Flags |= CF_LIMITSIZE;
	_fontSelector.nSizeMin = min;
	_fontSelector.nSizeMax = max;
}

void Selector::SetDefaultSelection (char const * faceName, int pointSize)
{
	_fontMaker.Init (pointSize, faceName);
	MatchCharSet ();
	Assert (_fontSelector.hDC == 0);
	_fontSelector.Flags |= CF_SCREENFONTS;
	_fontSelector.Flags |= CF_INITTOLOGFONTSTRUCT;
}

void Selector::SetDefaultSelection (Font::Maker const & defaultFont)
{
	_fontMaker = defaultFont;
	MatchCharSet ();
	_fontSelector.Flags |= CF_INITTOLOGFONTSTRUCT;
}

bool Selector::Select ()
{
	BOOL retValue = ::ChooseFont (&_fontSelector);
	if (retValue == FALSE)
	{
		DWORD err = ::CommDlgExtendedError ();
		if (err != 0)
			throw Win::CommDlgException (err, "Font Selection Dialog");
	}
	return retValue != FALSE;
}

void Selector::MatchCharSet ()
{
	Win::DesktopCanvas screen;
	// If DC provided use it else use screen DC
	HDC device = _fontSelector.hDC != 0 ? _fontSelector.hDC : screen.ToNative ();
	// Determine character set matching current keyboard layout
	_fontMaker.SetCharSet (Font::CharSet::Default);
    int result = ::EnumFontFamiliesEx (device,
									   reinterpret_cast<LOGFONT *>(&_fontMaker), 
									   &Selector::EnumFontFamCharSet, 
									   reinterpret_cast<LPARAM>(&_keyboard),
									   0);
	if (result == 0)
    {
        // Found font matching keyboard layout -- use its character set
		_fontMaker.SetCharSet (_keyboard.charSet);
		// Limit font selection only to fonts supporting the same character set
		// as curren keyboard layout
		_fontSelector.Flags |= CF_SELECTSCRIPT;
    }
}

int CALLBACK Selector::EnumFontFamCharSet (LOGFONT const * logFont,			// pointer to logical-font data
											   TEXTMETRIC const * fontMetrics,	// pointer to physical-font data
											   ULONG type,						// type of font
											   LPARAM lParam)					// application-defined data 
{
	struct keyboardInfo * keyboard = reinterpret_cast<struct keyboardInfo *>(lParam);
    CHARSETINFO fontCharSet;
    LOCALESIGNATURE keyboardCharSet;
    
    // Get character set info for log font passed in.
    if (::TranslateCharsetInfo (reinterpret_cast<DWORD *>(fontMetrics->tmCharSet),
								&fontCharSet, TCI_SRCCHARSET) != 0)
	{
		// Get code pages supported by the active keyboard layout
		// Low word of keyboard layout contains language identifier
		if (::GetLocaleInfo (MAKELCID(LOWORD(keyboard->layout), SORT_DEFAULT),
							 LOCALE_FONTSIGNATURE,	
							 reinterpret_cast<LPSTR>(&keyboardCharSet),
							 sizeof (LOCALESIGNATURE)) != 0)
		{
			// Compare keyboard code page and default font code page
			// stop enumerating if a match is found.
			if (fontCharSet.fs.fsCsb [0] & keyboardCharSet.lsCsbDefault [0])
			{
				// Remember font character set that matches keyboard
				keyboard->charSet = logFont->lfCharSet;
				return FALSE;
			}
		}
	}
	return TRUE;
}
