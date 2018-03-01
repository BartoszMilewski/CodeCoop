//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "FeedbackBar.h"
#include "UiManagerIds.h"

FeedbackBar::FeedbackBar (Win::Dow::Handle win)
{
	Win::StatusBarMaker statusMaker (win, STATUS_BAR_ID);
	statusMaker.Style () << Win::Style::Ex::ClientEdge;
	_statusBar.Reset (statusMaker.Create ());
	_statusBar.AddPart (40);	// for Activity notices
	_statusBar.AddPart (50);	// for Caption notices or Progress Bar
	_statusBar.AddPart (10);	// for Supplemental Activity notices

	Win::ProgressBarMaker barMaker (_statusBar);
	barMaker.Style () << Win::Style::Ex::ClientEdge;
	_progressBar = barMaker.Create ();
}

void FeedbackBar::Size (Win::Rect const & feedbackRect)
{
	_statusBar.ReSize (feedbackRect.Left (),
					   feedbackRect.Top (),
					   feedbackRect.Width (),
					   feedbackRect.Height ());
	Win::Rect rect;
	_statusBar.GetPartRect (rect, 1);
	// Progress bar if visible obscures second status bar part
	_progressBar.ReSize (rect.Left (), rect.Top (), rect.Width (), rect.Height ());
}

std::string FeedbackBar::GetCurrentActivity ()
{
	return _statusBar.GetString ();
}
