#if !defined (RICHDUMPWINDOW_H)
#define RICHDUMPWINDOW_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "DumpWin.h"

#include <Win/Win.h>
#include <Graph/Color.h>
#include <Ctrl/RichEditCtrl.h>

// RichDumpWindow uses RichEdit as its display device. It can translate DumpWindow styles
// to RichEdit character and paragraph formats.

class RichDumpWindow : public DumpWindow, public Win::RichEdit
{
public:
	RichDumpWindow ()
		: _charFormats (DumpWindow::styLast),
		  _paraFormats (DumpWindow::styLast),
		  _isDarkBkg (false),
		  _lfCrNeeded (false)
	{}

	void Create (Win::Dow::Handle frame, int id);
	void SetBackground (Win::Color color);
	void SetTextColor (Win::Color color);
	void RefreshFormats ();
	void AddScrollBars ();

	// Dump Window interface
	void PutLine (char const * line, DumpWindow::Style style);
	void PutLine (std::string const & line, DumpWindow::Style style);
	void PutLine (std::string const & line, EditStyle act, bool isFirstLine);

private:
	enum
	{
		Header2Indent = 2,		// 2 white spaces
		NormalIndent = 4,		// 4 white spaces
		CodeIndent = 20			// 20 white spaces
	};

private:
	void RefreshColors ();
	void DisplayLine (std::string const & line,
					  Win::RichEdit::CharFormat const & charFormat,
					  Win::RichEdit::ParaFormat const & paraFormat);

private:
	std::vector<Win::RichEdit::CharFormat>	_charFormats;
	std::vector<Win::RichEdit::ParaFormat>	_paraFormats;
	Win::Color								_bkgColor;
	Win::Color								_txtColor;
	bool									_isDarkBkg;
	bool									_lfCrNeeded;
};

#endif
