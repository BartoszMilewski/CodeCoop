//------------------------------------------------
// RichEditCtrl.cpp
// (c) Reliable Software 2002 -- 2003
// -----------------------------------------------

#include <WinLibBase.h>
#include "RichEditCtrl.h"

#include <Sys/Dll.h>
#include <Win/Message.h>
#include <Graph/Font.h>
#include <Graph/Canvas.h>

using namespace Win;
using namespace Notify;

RichEdit::RichEdit ()
	: _isV2 (true)
{
	Style () << Win::Style::Visible;
	Style () << Win::Style::Child;
	LoadCtrlDll ();
}

RichEdit::RichEdit (Win::Dow::Handle winParent, int id)
	: _isV2 (true)
{
	Style () << Win::Style::Visible;
	Style () << Win::Style::Child;
	LoadCtrlDll ();
	Init (winParent, id);
}

RichEdit::~RichEdit ()
{}

// Parent receives child messages, canvas is where the child paints itself
Win::Dow::Handle RichEdit::Create (Win::Dow::Handle winParent, Win::Dow::Handle canvas, int id)
{
	Win::Dow::Handle h = ::CreateWindowEx (0,						// extended window style
										   GetWinClassName (),		// pointer to registered class name
										   0,						// pointer to window name
										   _style.GetStyleBits (),	// window style
										   CW_USEDEFAULT,			// horizontal position of window
										   0,						// vertical position of window
										   CW_USEDEFAULT,			// window width  
										   0,						// window height
										   winParent.ToNative (),	// handle to parent or owner window
										   reinterpret_cast<HMENU>(id),// handle to menu, or child-window identifier
										   winParent.GetInstance (),// handle to application instance
										   0);						// pointer to window-creation data

	if (h.IsNull ())
		throw Win::Exception ("Internal error: Rich edit control creation failed.");

	::SetParent (h.ToNative (), canvas.ToNative ());
	Reset (h.ToNative ());
	return h;
}

int RichEdit::Millimeters2Twips (int millimeters)
{
	//  1440 twips = 1 inch
	//     1 inch  = 25,4 millimeters
	// 14400 twips = 254 millimeters
	return (millimeters * 14400) / 254;
}

int RichEdit::Points2Twips (int points)
{
	// 20 twips = 1 point
	return points * 20;
}

int RichEdit::GetCurrentPosition () const
{
	CHARRANGE range;
	Message msgRange (EM_EXGETSEL, 0, reinterpret_cast<LPARAM>(&range));
	SendMsg (msgRange);
	return range.cpMin;
}

void RichEdit::Select (int start, int stop)
{
	CHARRANGE range;
	range.cpMin = start;
	range.cpMax = stop;
	Message msgRange (EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
	SendMsg (msgRange);
}

void RichEdit::Append (char const * buf, CharFormat const & charFormat, ParaFormat const & paraFormat)
{
	// Appending paragraph longer them 8187 characters looses paragraph formating
	unsigned int const maxParaLength = 4096;
	unsigned int totalLength = strlen (buf);
	unsigned int paraBegin = 0;
	while (totalLength != 0)
	{
		unsigned int paraLength = totalLength > maxParaLength ? maxParaLength : totalLength;
		std::string tmp;
		tmp.assign (&buf [paraBegin], paraLength);
		// Move insetion point to the text end
		Select (EndPos (), EndPos ());
		// Set new format
		SetCharFormat (charFormat);
		SetParaFormat (paraFormat);
		// Insert text at edit end
		Message insert (EM_REPLACESEL, 0, reinterpret_cast<LPARAM> (tmp.c_str ()));
		SendMsg (insert);
		paraBegin += paraLength;
		totalLength -= paraLength;
	}
}

void RichEdit::SetCharFormat (RichEdit::CharFormat const & charFormat, bool all)
{
	if (_isV2)
	{
		Message msg (EM_SETCHARFORMAT, all ? SCF_ALL : SCF_SELECTION, reinterpret_cast<LPARAM>(&charFormat));
		SendMsg (msg);
	}
	else
	{
		CHARFORMAT formatV1;
		formatV1.cbSize = (sizeof (CHARFORMAT));
		formatV1.dwMask = charFormat.dwMask;
		formatV1.dwEffects = charFormat.dwEffects;
		formatV1.yHeight = charFormat.yHeight;
		formatV1.yOffset = charFormat.yOffset;
		formatV1.crTextColor = charFormat.crTextColor;
		formatV1.bCharSet = charFormat.bCharSet;
		formatV1.bPitchAndFamily = charFormat.bPitchAndFamily;
		memcpy (formatV1.szFaceName, charFormat.szFaceName, sizeof (formatV1.szFaceName));
		Message msg (EM_SETCHARFORMAT, all ? SCF_ALL : SCF_SELECTION, reinterpret_cast<LPARAM>(&formatV1));
		SendMsg (msg);
	}
}

void RichEdit::SetParaFormat (RichEdit::ParaFormat const & paraFormat)
{
	if (_isV2)
	{
		Message msg (EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&paraFormat));
		SendMsg (msg);
	}
	else
	{
		PARAFORMAT formatV1;
		formatV1.cbSize = (sizeof (PARAFORMAT));
		formatV1.dwMask = paraFormat.dwMask;
		formatV1.wNumbering = paraFormat.wNumbering;
		formatV1.wEffects = 0;
		formatV1.dxStartIndent = paraFormat.dxStartIndent;
		formatV1.dxRightIndent = paraFormat.dxRightIndent;
		formatV1.dxOffset = paraFormat.dxOffset;
		formatV1.wAlignment = paraFormat.wAlignment;
		formatV1.cTabCount = paraFormat.cTabCount;
		memcpy (formatV1.rgxTabs, paraFormat.rgxTabs, sizeof (formatV1.rgxTabs));
		Message msg (EM_SETCHARFORMAT, 0, reinterpret_cast<LPARAM>(&formatV1));
		SendMsg (msg);
	}
}

void RichEdit::SetBackground (Win::Color color)
{
	Message msg (EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(color.ToNative ()));
	SendMsg (msg);
}

void RichEdit::EnableResizeRequest ()
{
	Message msg (EM_SETEVENTMASK, 0, ENM_REQUESTRESIZE);
	SendMsg (msg);
}

void RichEdit::RequestResize ()
{
	Message msg (EM_REQUESTRESIZE);
	SendMsg (msg);
}

void RichEdit::LoadCtrlDll ()
{
	// Find out what rich edit version is present in the system
	// Loading control Dll registers appropriate window class
	_editDll.reset (new Dll ("Riched20.dll", true));	// Quiet load library
	if (_editDll->IsNull ())
	{
		// Rich edit 2.0 not present -- try loading rich edit 1.0
		_isV2 = false;
		_editDll.reset (new Dll ("Riched32.dll"));		// Not quiet load library, will throw exception if dll not found
	}
}

char const * RichEdit::GetWinClassName () const
{
	if (_isV2)
		return RICHEDIT_CLASS;
	else
		return "RichEdit";
}

RichEdit::CharFormat::CharFormat ()
{
	memset (this, 0, sizeof (CharFormat));
	cbSize = sizeof (CharFormat);
}

RichEdit::CharFormat::CharFormat (RichEdit::CharFormat const & format)
{
	memcpy (this, &format, sizeof (CharFormat));
}

void RichEdit::CharFormat::SetFont (int pointSize, Font::Descriptor const & font)
{
	SetFontSize (pointSize);
	SetCharSet (font.lfCharSet);
	SetPitchAndFamily (font.lfPitchAndFamily);
	SetFaceName (&font.lfFaceName [0]);
	SetWeight (font.lfWeight);
	SetBold (font.lfWeight == FW_BOLD);
	if (font.lfItalic != 0)
		SetItalic ();
	if (font.lfStrikeOut != 0)
		SetStrikeOut ();
}

void RichEdit::CharFormat::SetFont (int pointSize, std::string const & faceName)
{
	Font::Maker fontMaker (pointSize, faceName.c_str ());
	SetFont (pointSize, fontMaker);
}

void RichEdit::CharFormat::SetFontSize (int pointSize)
{
	yHeight = Points2Twips (pointSize);
	dwMask |= CFM_SIZE;
}

void RichEdit::CharFormat::SetCharSet (unsigned char charSet)
{
	bCharSet = charSet;
	dwMask |= CFM_CHARSET;
}

void RichEdit::CharFormat::SetPitchAndFamily (unsigned char pitchAndFamily)
{
	bPitchAndFamily = pitchAndFamily;	// Note, no need to set dwMask
}

void RichEdit::CharFormat::SetFaceName (char const * faceName)
{
    Assert (strlen(faceName) < LF_FACESIZE);
    strcpy (szFaceName, faceName);
	dwMask |= CFM_FACE;
}

void RichEdit::CharFormat::SetWeight (int weight)
{
	wWeight = weight;
	dwMask |= CFM_WEIGHT;
}

void RichEdit::CharFormat::SetTextColor (Win::Color color)
{
	dwEffects &= ~CFE_AUTOCOLOR;
	crTextColor = color.ToNative ();
	dwMask |= CFM_COLOR;
}

void RichEdit::CharFormat::SetBackColor (Win::Color color)
{
	dwEffects &= ~CFE_AUTOBACKCOLOR;
	crBackColor = color.ToNative ();
	dwMask |= CFM_BACKCOLOR;
}

void RichEdit::CharFormat::SetBold (bool bold)
{
	dwMask |= CFM_BOLD;
	if (bold)
		dwEffects |= CFE_BOLD;
	else
		dwEffects &= ~CFE_BOLD;
}

void RichEdit::CharFormat::SetItalic (bool italic)
{
	dwMask |= CFM_ITALIC;
	if (italic)
		dwEffects |= CFE_ITALIC;
	else
		dwEffects &= ~CFE_ITALIC;
}

void RichEdit::CharFormat::SetStrikeOut (bool strikeOut)
{
	dwMask |= CFM_STRIKEOUT;
	if (strikeOut)
		dwEffects |= CFE_STRIKEOUT;
	else
		dwEffects &= ~CFE_STRIKEOUT;
}

RichEdit::ParaFormat::ParaFormat ()
{
	memset (this, 0, sizeof (ParaFormat));
	cbSize = sizeof (ParaFormat);

}

// Set the same left ident for the first and subsequent lines
void RichEdit::ParaFormat::SetLeftIdent (int twips, bool absolute)
{
	// Indentation of the paragraph's first line, in twips.
	// The indentation of subsequent lines depends on the dxOffset member.
	dxStartIndent = twips;
	if (absolute)
		dwMask |= PFM_STARTINDENT;
	else
		dwMask |= PFM_OFFSETINDENT;
	// Indentation of the second and subsequent lines, relative to the indentation of the first line, in twips.
	// The first line is indented if this member is negative or outdented if this member is positive.
	dxOffset = 0;
	dwMask |= PFM_OFFSET;
}

void RichEdit::ParaFormat::SetSpaceBefore (int twips)
{
	dySpaceBefore = twips;
	dwMask |= PFM_SPACEBEFORE;
}

void RichEdit::ParaFormat::SetSpaceAfter (int twips)
{
	dySpaceAfter = twips;
	dwMask |= PFM_SPACEAFTER;
}

void RichEdit::ParaFormat::SetTabStopEvery (int tabTwips)
{
	cTabCount = MAX_TAB_STOPS;
	for (int i = 1; i <= MAX_TAB_STOPS; ++i)
	{
		rgxTabs [i - 1] = i * tabTwips;
	}
	dwMask |= PFM_TABSTOPS;
}

bool RichEditHandler::OnNotify (NMHDR * hdr, long & result)
{
	// hdr->code
	// hdr->idFrom;
	// hdr->hwndFrom;
	switch (hdr->code)
	{
	case EN_REQUESTRESIZE:
		{
			REQRESIZE * request = reinterpret_cast<REQRESIZE *>(hdr);
			return OnRequestResize (request->rc);
		}
	}
	return false;
}
