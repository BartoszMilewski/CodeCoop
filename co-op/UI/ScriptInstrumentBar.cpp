//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ScriptInstrumentBar.h"
#include "UiManagerIds.h"

#include <Ctrl/Edit.h>
#include <Dbg/Out.h>

ScriptInstrumentBar::ScriptInstrumentBar (Win::Dow::Handle parentWin,
										  Cmd::Vector & cmdVector,
										  Cmd::Executor & executor)
	: Tool::DynamicRebar (parentWin, executor, Tool::Buttons, SCRIPT_TOOLBOX_ID, SCRIPT_TOOLBOX_CTRL_ID)
{
	CreateToolBands (cmdVector);
	// Add non-button controls to the selected button bands
	CreateDisplayTools (parentWin, executor);
	LayoutChange (Tool::BandLayoutTable [Tool::HistoryScriptDetails]);
}

void ScriptInstrumentBar::CreateToolBands (Cmd::Vector & cmdVector)
{
	// Create all button bands
	for (unsigned i = 0; Tool::ScriptBands [i].bandId != Tool::InvalidBandId; ++i)
	{
		AddButtonBand (Tool::ScriptBands [i], cmdVector);
	}
}

void ScriptInstrumentBar::CreateDisplayTools (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::ScriptCommentBandId);
	Assert (band != 0);
	_scriptCommentDisplay.reset (new InfoDisplayCtrl (SCRIPT_DETAILS_COMMENT_DISPLAY_ID,
													  parentWin,
													  band->GetWindow (),
													  executor));
	band->SetCaption ("Details of:");
	band->RegisterToolTip (_scriptCommentDisplay->GetWindow (), parentWin, "");
}

Control::Handler * ScriptInstrumentBar::GetControlHandler (Win::Dow::Handle winFrom, 
														   unsigned idFrom) throw ()
{
	if (_scriptCommentDisplay->IsHandlerFor (idFrom))
		return _scriptCommentDisplay.get ();
	else
		return Tool::DynamicRebar::GetControlHandler (winFrom, idFrom);
}

void ScriptInstrumentBar::SetFont (Font::Descriptor const & font)
{
	_scriptCommentDisplay->SetFont (font);
}

void ScriptInstrumentBar::RefreshTextField (std::string const & comment)
{
	unsigned int eolPos = comment.find_first_of ("\r\n");
	if (eolPos != std::string::npos)
	{
		// Multi-line caption.	Display first line (add "..." at the end)
		// Complete caption will be displayed as tool tip.
		std::string firstLine (&comment [0], eolPos);
		firstLine += " ...";
		_scriptCommentDisplay->SetText (firstLine);
	}
	else
	{
		_scriptCommentDisplay->SetText (comment);
	}
	Tool::ButtonBand * band = GetButtonBand (Tool::ScriptCommentBandId);
	band->RegisterToolTip (_scriptCommentDisplay->GetWindow (), GetParentWin (), comment);
}

bool ScriptInstrumentBar::OnChildSize (unsigned bandIdx,
									   unsigned bandId,
									   Win::Rect & newChildRect,
									   Win::Rect const & newBandRect) throw ()
{
	dbg << "--> ScriptInstrumentBar::OnChildSize" << std::endl;
	dbg << "    Band idx: " << std::dec << bandIdx << "; band id: " << std::dec << bandId << std::endl;
	dbg << "    Child new rect: " << newChildRect << std::endl;
	dbg << "    Band new rect: " << newBandRect << std::endl;
	if (bandId == Tool::ScriptCommentBandId)
	{
		Tool::ButtonBand const * band = GetButtonBand (bandId);
		unsigned buttonExtent = band->GetButtonExtent ();
		Win::Rect rect;
		rect.left = buttonExtent + Tool::DynamicRebar::fudgeFactor;
		rect.right = newChildRect.right - newChildRect.left - Tool::DynamicRebar::fudgeFactor;
		rect.top += Tool::DynamicRebar::fudgeFactor;
		rect.bottom = rect.top + Tool::DefaultButtonWidth;
		_scriptCommentDisplay->Move (rect);
	}
	dbg << "<-- ScriptInstrumentBar::OnChildSize" << std::endl;
	return true;
}
