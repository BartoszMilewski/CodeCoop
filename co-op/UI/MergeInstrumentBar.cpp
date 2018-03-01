//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "MergeInstrumentBar.h"
#include "UiManagerIds.h"
#include "UiStrings.h"

#include <Ctrl/Edit.h>
#include <Dbg/Out.h>

MergeInstrumentBar::MergeInstrumentBar (Win::Dow::Handle parentWin,
										  Cmd::Vector & cmdVector,
										  Cmd::Executor & executor)
	: Tool::DynamicRebar (parentWin, executor, Tool::Buttons, MERGE_TOOLBOX_ID, MERGE_TOOLBOX_CTRL_ID)
{
	CreateToolBands (cmdVector);
	// Add non-button controls to the selected button bands
	CreateTargetProjectTool (parentWin, executor);
	CreateMergeTypeTool (parentWin, executor);
	CreateDisplayTool (parentWin, executor);
	LayoutChange (Tool::BandLayoutTable [Tool::ProjectMergeTools]);
}

void MergeInstrumentBar::CreateToolBands (Cmd::Vector & cmdVector)
{
	// Create all button bands
	for (unsigned i = 0; Tool::ProjectMergeBands [i].bandId != Tool::InvalidBandId; ++i)
	{
		AddButtonBand (Tool::ProjectMergeBands [i], cmdVector);
	}
}

void MergeInstrumentBar::CreateTargetProjectTool (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::TargetProjectBandId);
	Assert (band != 0);
	band->SetCaption ("Target project:");
	_targetProjectCtrl.reset (new TargetProjectCtrl (TARGET_PROJECT_DISPLAY_ID,
													 parentWin,			// Gets notifications
													 band->GetWindow (),// Displays drop down
													 executor));
	// Add drop down window to the tool bar tip control.
	// Send tool tip requests to the parent window.
	std::string tip ("Select target project for the merge");

	band->RegisterToolTip (_targetProjectCtrl->GetView (), parentWin, tip);
	band->RegisterToolTip (_targetProjectCtrl->GetEditWindow (), parentWin, tip);
}

void MergeInstrumentBar::CreateMergeTypeTool (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::MergeTypeBandId);
	Assert (band != 0);
	band->SetCaption ("Merge type:");
	_mergeTypeCtrl.reset (new MergeTypeCtrl (MERGE_TYPE_DISPLAY_ID,
											 parentWin,			// Gets notifications
											 band->GetWindow (),// Displays drop down
											 executor));
	// Add drop down window to the tool bar tip control.
	// Send tool tip requests to the parent window.
	std::string tip ("Select merge type");

	band->RegisterToolTip (_mergeTypeCtrl->GetView (), parentWin, tip);
	band->RegisterToolTip (_mergeTypeCtrl->GetEditWindow (), parentWin, tip);
	_mergeTypeCtrl->Refresh (MergeTypeEntries);
}

void MergeInstrumentBar::CreateDisplayTool (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::MergedVersionBandId);
	Assert (band != 0);
	_scriptCommentDisplay.reset (new InfoDisplayCtrl (MERGED_VERSION_DISPLAY_ID,
								 parentWin,
								 band->GetWindow (),
								 executor));
	band->SetCaption ("Merged version:");
	band->RegisterToolTip (_scriptCommentDisplay->GetWindow (), parentWin, "");
}

Control::Handler * MergeInstrumentBar::GetControlHandler (Win::Dow::Handle winFrom, 
														   unsigned idFrom) throw ()
{
	if (_targetProjectCtrl->IsHandlerFor (idFrom))
		return _targetProjectCtrl.get ();
	else if (_mergeTypeCtrl->IsHandlerFor (idFrom))
		return _mergeTypeCtrl.get ();
	else if (_scriptCommentDisplay->IsHandlerFor (idFrom))
		return _scriptCommentDisplay.get ();
	else
		return Tool::DynamicRebar::GetControlHandler (winFrom, idFrom);
}

void MergeInstrumentBar::SetFont (Font::Descriptor const & font)
{
	_targetProjectCtrl->SetFont (font);
	_mergeTypeCtrl->SetFont (font);
	_scriptCommentDisplay->SetFont (font);
}

void MergeInstrumentBar::RefreshTextField (std::string const & text)
{
	if (text.empty ())
		return;

	// Text format - <hint list> | <script comment> | <merge type>
	std::string::size_type markerPos = text.find ('|');
	Assert (markerPos != std::string::npos);
	std::string hintList (text.substr (0, markerPos));
	std::string::size_type markerPos1 = text.find ('|', markerPos + 1);
	std::string comment;
	std::string mergeType;
	if (markerPos1 != std::string::npos)
	{
		if (markerPos + 1 != markerPos1)
			comment = text.substr (markerPos + 1, markerPos1 - markerPos - 1);
		mergeType = text.substr (markerPos1 + 1);
	}
	else
	{
		comment = text.substr (markerPos + 1);
	}

	markerPos = hintList.find ("*");
	if (markerPos != std::string::npos)
		hintList.replace (markerPos, 1, SelectBranch);
	_targetProjectCtrl->Refresh (hintList);

	std::string::size_type eolPos = comment.find_first_of ("\r\n");
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
	Tool::ButtonBand * band = GetButtonBand (Tool::MergedVersionBandId);
	Assert (band != 0);
	band->RegisterToolTip (_scriptCommentDisplay->GetWindow (), GetParentWin (), comment);

	if (!mergeType.empty ())
		_mergeTypeCtrl->Select (mergeType);
}

bool MergeInstrumentBar::OnChildSize (unsigned bandIdx,
									  unsigned bandId,
									  Win::Rect & newChildRect,
									  Win::Rect const & newBandRect) throw ()
{
	dbg << "--> MergeInstrumentBar::OnChildSize" << std::endl;
	dbg << "    Band idx: " << std::dec << bandIdx << "; band id: " << std::dec << bandId << std::endl;
	dbg << "    Child new rect: " << newChildRect << std::endl;
	dbg << "    Band new rect: " << newBandRect << std::endl;
	Tool::ButtonBand const * band = GetButtonBand (bandId);
	Assert (band != 0);
	unsigned buttonExtent = band->GetButtonExtent ();
	Win::Rect rect;
	rect.left = buttonExtent + Tool::DynamicRebar::fudgeFactor;
	rect.right = newChildRect.right - newChildRect.left - Tool::DynamicRebar::fudgeFactor;
	if (bandId == Tool::MergedVersionBandId)
	{
		rect.top = 2 * Tool::DynamicRebar::fudgeFactor;
		rect.bottom = rect.top + Tool::DefaultButtonWidth;
		_scriptCommentDisplay->Move (rect);
	}
	if (bandId == Tool::TargetProjectBandId || bandId == Tool::MergeTypeBandId)
	{
		Win::Dow::Handle parentWin = GetParentWin ();
		Win::ClientRect parentRect (parentWin);
		rect.top = Tool::DynamicRebar::fudgeFactor;
		rect.bottom = rect.top + (parentRect.Height () - 2 * Tool::DynamicRebar::fudgeFactor);
		if (bandId == Tool::TargetProjectBandId)
			_targetProjectCtrl->GetView ().Move (rect);
		else
			_mergeTypeCtrl->GetView ().Move (rect);
	}
	dbg << "<-- MergeInstrumentBar::OnChildSize" << std::endl;
	return true;
}
