//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "RichDumpWin.h"
#include "Registry.h"

#include <Win/Metrics.h>
#include <Graph/Font.h>

void RichDumpWindow::Create (Win::Dow::Handle frame, int id)
{
	// Create rich edit control
	Win::RichEdit::Style () << Win::Edit::Style::ReadOnly;
	Win::RichEdit::Style () << Win::Edit::Style::MultiLine;
	Win::RichEdit::Create (frame, frame, id);
	_lfCrNeeded = false;
	SetBackground (Win::Color::Window ());
	RefreshFormats ();
}

void RichDumpWindow::AddScrollBars ()
{
	Win::RichEdit::Style () << Win::Style::AddVScrollBar;
	Win::RichEdit::Style () << Win::Style::AddHScrollBar;
}

void RichDumpWindow::SetBackground (Win::Color color)
{
	_bkgColor = color;
	int intensity = _bkgColor.R () + _bkgColor.G () + _bkgColor.B ();
	_isDarkBkg = (intensity > 3 * 128) ? false : true;
	_txtColor = _isDarkBkg ? Win::Color (255, 255, 255) // white
						   : Win::Color (0, 0, 0);		// black
	Win::RichEdit::SetBackground (_bkgColor);
	RefreshColors ();
}

void RichDumpWindow::SetTextColor (Win::Color color)
{
	_txtColor = color;
	RefreshColors ();
}

void RichDumpWindow::RefreshFormats ()
{
	NonClientMetrics metrics;
	Assert (metrics.IsValid ());
	Font::Maker normalFont (metrics.GetMessageFont ());
	int normalPointSize = normalFont.GetPointSize ();

	// Style header 1
	_charFormats [styH1].SetFont (normalPointSize + 5, normalFont);
	_charFormats [styH1].SetBold ();

	// Style header 2
	_charFormats [styH2].SetFont (normalPointSize + 3, normalFont);

	// Style normal
	_charFormats [styNormal].SetFont (normalPointSize + 1, normalFont);

	// Style code -- use the same font as used by Differ
	Font::Maker codeFont;
	Registry::UserDifferPrefs prefs;
	unsigned tabSize = 4;
	if (!prefs.GetFont (tabSize, codeFont))
		codeFont.Init (9, "Courier New");

	_charFormats [styCodeFirst].SetFont (codeFont.GetPointSize (), codeFont);
	_charFormats [styCodeNext].SetFont (codeFont.GetPointSize (), codeFont);

	RefreshColors ();

	// Set paragraph formats -- all measurements in twips
	// 20 twips == 1 point
	// 1440 twips == 1 inch
	// 14400 twips == 254 millimeters
	Win::DesktopCanvas desktop;
	Font::Holder codeFontHolder (desktop, codeFont.Create ());
	int whiteSpaceWidthTwips = codeFontHolder.GetCharWidthTwips (' ', codeFont.GetPointSize ());
	Win::RichEdit::ParaFormat * paraFmt;

	// Style header 1
	paraFmt = &_paraFormats [styH1];
	paraFmt->SetLeftIdent (whiteSpaceWidthTwips);
	paraFmt->SetSpaceBefore (170);	// Approx. 3 millimeters
	paraFmt->SetSpaceAfter (113);	// Approx. 2 millimeters
	paraFmt->SetTabStopEvery (tabSize * whiteSpaceWidthTwips);	// Every tabSize white space

	// Style header 2
	paraFmt = &_paraFormats [styH2];
	paraFmt->SetLeftIdent (Header2Indent * whiteSpaceWidthTwips);
	paraFmt->SetSpaceBefore (170);
	paraFmt->SetSpaceAfter (113);
	paraFmt->SetTabStopEvery (tabSize * whiteSpaceWidthTwips);	// Every tabSize white space

	// Style normal
	paraFmt = &_paraFormats [styNormal];
	paraFmt->SetLeftIdent (NormalIndent * whiteSpaceWidthTwips);
	paraFmt->SetSpaceBefore (0);
	paraFmt->SetSpaceAfter (0);
	paraFmt->SetTabStopEvery (tabSize * whiteSpaceWidthTwips);	// Every tabSize white space

	// Style code -- first line
	paraFmt = &_paraFormats [styCodeFirst];
	paraFmt->SetLeftIdent (NormalIndent * whiteSpaceWidthTwips);
	paraFmt->SetSpaceBefore (0);
	paraFmt->SetSpaceAfter (0);
	paraFmt->SetTabStopEvery (tabSize * whiteSpaceWidthTwips);	// Every tabSize white space

	// Style code -- subsequent lines
	paraFmt = &_paraFormats [styCodeNext];
	// Subsequent lines are indented 20 white space string width
	paraFmt->SetLeftIdent (CodeIndent * whiteSpaceWidthTwips);
	paraFmt->SetSpaceBefore (0);
	paraFmt->SetSpaceAfter (0);
	paraFmt->SetTabStopEvery (tabSize * whiteSpaceWidthTwips);	// Every tabSize white space
}

void RichDumpWindow::PutLine (std::string const & line, DumpWindow::Style style)
{
	DisplayLine (line, _charFormats [style], _paraFormats [style]);
}

void RichDumpWindow::PutLine (char const * line, DumpWindow::Style style)
{
	DisplayLine (line, _charFormats [style], _paraFormats [style]);
}

void RichDumpWindow::PutLine (std::string const & line, EditStyle act, bool isFirstLine)
{
	int currentPosition;
	if (isFirstLine)
	{
		Select (EndPos (), EndPos ());
		currentPosition = GetCurrentPosition ();
	}

	Win::RichEdit::CharFormat charFormat (isFirstLine ? _charFormats [styCodeFirst] : _charFormats [styCodeNext]);
	Win::Color bkgRgb;
	switch (act.GetAction ())
	{
	case EditStyle::actNone:
		bkgRgb = _isDarkBkg ? Win::Color (0, 0, 0): Win::Color (0xcc, 0xcc, 0xff); // light blue
		break;
	case EditStyle::actDelete:
		bkgRgb = _isDarkBkg ? Win::Color (0x33, 0, 0): Win::Color (0xff, 0xcc, 0xcc); // red
		break;
	case EditStyle::actInsert:
		bkgRgb = _isDarkBkg ? Win::Color (0x33, 0x33, 0) : Win::Color (0xff, 0xff, 0xcc); // yellow
		break;
	case EditStyle::actCut:
	case EditStyle::actPaste:
	default:
		bkgRgb = _isDarkBkg ? Win::Color (0, 0, 0x33) : Win::Color (0xcc, 0xcc, 0xff); // blue
		break;
	}
	charFormat.SetBackColor (bkgRgb);
	DisplayLine (line, charFormat, isFirstLine ? _paraFormats [styCodeFirst] : _paraFormats [styCodeNext]);
	if (isFirstLine)
	{
		// Make sure that first line's CodeIndent first characters are on the default window background
		Win::Color bkgColor = Win::Color::Window ();
		charFormat.SetBackColor (bkgColor);
		Select (currentPosition, currentPosition + CodeIndent - NormalIndent);
		SetCharFormat (charFormat);
	}
}

void RichDumpWindow::RefreshColors ()
{
	_charFormats [styH1].SetTextColor (_txtColor);
	_charFormats [styH1].SetBackColor (_bkgColor);
	_charFormats [styH2].SetTextColor (_txtColor);
	_charFormats [styH2].SetBackColor (_bkgColor);
	_charFormats [styNormal].SetTextColor (_txtColor);
	_charFormats [styNormal].SetBackColor (_bkgColor);
	_charFormats [styCodeFirst].SetTextColor (_txtColor);
	_charFormats [styCodeFirst].SetBackColor (_bkgColor);
	_charFormats [styCodeNext].SetTextColor (_txtColor);
	_charFormats [styCodeNext].SetBackColor (_bkgColor);
}

void RichDumpWindow::DisplayLine (std::string const & line,
								  Win::RichEdit::CharFormat const & charFormat,
								  Win::RichEdit::ParaFormat const & paraFormat)
{
	if (_lfCrNeeded)
		Append ("\r\n");
	Append (line.c_str (), charFormat, paraFormat);
	_lfCrNeeded = (line.find_first_of ("\r\n") == std::string::npos);
}
