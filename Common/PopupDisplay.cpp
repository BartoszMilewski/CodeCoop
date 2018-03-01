//---------------------------------------
//  PopupDisplay.cpp
//  (c) Reliable Software, 2002
//---------------------------------------

#include "precompiled.h"
#include "PopupDisplay.h"
#include "RichDumpWin.h"
#include "DisplayFrameCtrl.h"
#include "FormattedText.h"
#include "Global.h"

#include <Win/WinMaker.h>
#include <Win/Geom.h>
#include <Win/Metrics.h>

void PopupDisplay::Show (FormattedText const & text,
						 Win::Point & anchorPoint,
						 int maxWidth,
						 int maxHeight)
{
	// Create display frame
	Win::PopupMaker frameMaker (ListWindowClassName, _appWin.GetInstance ());
	frameMaker.Style () << Win::Style::AddBorder;
	frameMaker.SetOwner (_appWin);
	std::unique_ptr<DisplayFrameController> ctrl (new DisplayFrameController ());
	DisplayFrameController * frameCtrl = ctrl.get ();
	_frame = frameMaker.Create (std::move(ctrl));
	frameCtrl->CloseOnLostFocus ();
	RichDumpWindow * display = frameCtrl->GetRichDumpWindow ();
	display->SetBackground (Win::Color::ToolTipBkg ());
	// Dump formatted text
	for (FormattedText::Sequencer seq (text); !seq.AtEnd (); seq.Advance ())
	{
		display->PutLine (seq.GetLine (), seq.GetStyle ());
	}
	// Get formatting rectange after inserting all lines
	display->EnableResizeRequest ();
	display->RequestResize ();
	Win::Rect const & rect = frameCtrl->GetResizeRequest ();
	int finalWidth = rect.Width ();
	int finalHeight = rect.Height ();
	if (maxWidth < 500)
		maxWidth = 500;
	if (finalWidth > maxWidth)
	{
		// We have to start again, but this time we will
		// word-wrap too long lines.
		_frame.Destroy ();
		ctrl.reset (new DisplayFrameController (false));	// This time, no scroll bars
		frameCtrl = ctrl.get ();
		_frame = frameMaker.Create(std::move(ctrl));
		frameCtrl->CloseOnLostFocus ();
		display = frameCtrl->GetRichDumpWindow ();
		display->SetBackground (Win::Color::ToolTipBkg ());
		// Set formatting rectangle for word-wrap
		// Anchor point defines upper right corner
		Win::Rect fmtRect (anchorPoint.x - maxWidth,	// left
						   anchorPoint.y,		 		// top
						   anchorPoint.x,				// right
						   anchorPoint.y + maxHeight);	// bottom
		_frame.MoveDelayPaint (fmtRect.Left (), fmtRect.Top (), fmtRect.Width (), fmtRect.Height ());
		// Dump formatted text
		for (FormattedText::Sequencer seq (text); !seq.AtEnd (); seq.Advance ())
		{
			display->PutLine (seq.GetLine (), seq.GetStyle ());
		}
		// Get formatting rectange after inserting all lines with word-wrap
		display->EnableResizeRequest ();
		display->RequestResize ();
		Win::Rect const & rect = frameCtrl->GetResizeRequest ();
		finalWidth = rect.Width ();
		finalHeight = rect.Height ();
	}
	// Position popup display over parent
	// Anchor point defines upper right corner
	NonClientMetrics metrics;
	int scrollWidth = metrics.IsValid () ? 3 * metrics.GetScrollWidth () : 36;
	Win::Rect popupRect (anchorPoint.x - finalWidth - scrollWidth,	// left
						 anchorPoint.y,		 				// top
						 anchorPoint.x,						// right
						 anchorPoint.y + finalHeight + 6);	// bottom
	_frame.Move (popupRect.Left (), popupRect.Top (), popupRect.Width (), popupRect.Height ());
	_frame.Show ();
}
