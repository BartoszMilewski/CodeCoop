//------------------------------------
//  (c) Reliable Software, 2005 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "InstrumentBar.h"
#include "UiManagerIds.h"

#include <Dbg/Out.h>

InstrumentBar::InstrumentBar (Win::Dow::Handle parentWin,
							  Cmd::Vector & cmdVector,
							  Cmd::Executor & executor)
	: Tool::DynamicRebar (parentWin, executor, Tool::Buttons, TOOLBOX_ID, TOOLBOX_CTRL_ID)
{
	CreateToolBands (cmdVector);
	// Add non-button controls to the selected button bands
	CreateHistoryFilterTool (parentWin, executor);
	CreateUrlTool (parentWin, executor);
	CreateDisplayTools (parentWin);
}

void InstrumentBar::CreateToolBands (Cmd::Vector & cmdVector)
{
	// Create all button bands
	for (unsigned i = 0; Tool::Bands [i].bandId != Tool::InvalidBandId; ++i)
	{
		AddButtonBand (Tool::Bands [i], cmdVector);
	}
}

void InstrumentBar::CreateHistoryFilterTool (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::HistoryFilterBandId);
	Assert (band != 0);
	band->SetCaption ("History of:");
	_dropDownCtrl.reset (new HistoryFilterCtrl (HISTORY_FILTER_DISPLAY_ID,
												parentWin,			// Gets notifications
												band->GetWindow (),	// Displays drop down
												executor));
	// Add drop down window to the tool bar tip control.
	// Send tool tip requests to the parent window.
	std::string tip ("Enter file or folder names to see their history.\n"
		"Enter global ID of a file in angle brackets <> to see its history.\n"
		"Enter right angle bracket > followed by script ID  to select that script.\n"
		"Enter colon followed by query word or phrase to search script comments\n"
		"For instance, to see all versions that have \"bug fix\" in their\n"
		"comment, enter :bug fix");

	band->RegisterToolTip (_dropDownCtrl->GetView (), parentWin, tip);
	band->RegisterToolTip (_dropDownCtrl->GetEditWindow (), parentWin, tip);
}

void InstrumentBar::CreateUrlTool (Win::Dow::Handle parentWin, Cmd::Executor & executor)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::UrlBandId);
	Assert (band != 0);
	band->SetCaption ("Address:");
	_urlDisplayCtrl.reset (new UrlCtrl (URL_DISPLAY_ID,
										parentWin,			// Gets notifications
										band->GetWindow (),	// Displays drop down
										executor));

	band->RegisterToolTip (_dropDownCtrl->GetView (), parentWin, "Current page address");
	band->RegisterToolTip (_dropDownCtrl->GetEditWindow (), parentWin, "Type in page address");
}

void InstrumentBar::CreateDisplayTools (Win::Dow::Handle parentWin)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::CurrentFolderBandId);
	Assert (band != 0);
	band->SetCaption ("Current folder:");

	{
		Win::InfoDisplayMaker displayMaker (parentWin, CURRENT_FOLDER_DISPLAY_ID);
		_currentFolderDisplay.Reset (displayMaker.Create (band->GetWindow ()));
		_currentFolderDisplay.Show ();
	}
	// Add script comment window to the tool bar tip control.
	// Send tool tip requests to the parent window.
	band->RegisterToolTip (_currentFolderDisplay, parentWin, "Current Project Folder");

	band = GetButtonBand (Tool::ScriptCommentBandId);
	Assert (band != 0);
	{
		Win::InfoDisplayMaker displayMaker (parentWin, SCRIPT_COMMENT_DISPLAY_ID);
		_scriptCommentDisplay.Reset (displayMaker.Create (band->GetWindow ()));
		_scriptCommentDisplay.Show ();
	}
	band->RegisterToolTip (_scriptCommentDisplay, parentWin, "");

	band = GetButtonBand (Tool::CheckinStatusBandId);
	Assert (band != 0);
	{
		Win::InfoDisplayMaker displayMaker (parentWin, CHECKIN_STATUS_DISPLAY_ID);
		_checkinStatusDisplay.Reset (displayMaker.Create (band->GetWindow ()));
		_checkinStatusDisplay.Show ();
	}
}

Control::Handler * InstrumentBar::GetControlHandler (Win::Dow::Handle winFrom, 
													 unsigned idFrom) throw ()
{
	if (_dropDownCtrl->IsHandlerFor (idFrom))
		return _dropDownCtrl.get ();
	else if (_urlDisplayCtrl->IsHandlerFor (idFrom))
		return _urlDisplayCtrl.get ();
	else
		return Tool::DynamicRebar::GetControlHandler (winFrom, idFrom);
}

void InstrumentBar::RefreshTextField (std::string const & text)
{
	switch (_curBarLayout)
	{
	case Tool::BrowseFiles:
		RefreshCurrentFolder (text);
		break;

	case Tool::BrowseCheckInArea:
		RefreshCheckinStatus (text);
		break;

	case Tool::BrowseMailbox:
	case Tool::BrowseSyncArea:
		RefreshScriptComment (text);
		break;

	case Tool::BrowseHistory:
	case Tool::BrowseProjectMerge:
		RefreshFilter (text);
		break;
	case Tool::BrowseWiki:
		_urlDisplayCtrl->RefreshUrlList (text);
		break;
	}
}

std::string InstrumentBar::GetEditText () const
{
	if (_curBarLayout == Tool::BrowseWiki)
		return _urlDisplayCtrl->GetEditWindow ().GetText ();

	return std::string ();
}


void InstrumentBar::LayoutChange (Tool::BarLayout newLayout)
{
	_curBarLayout = newLayout;
	if (_curBarLayout == Tool::BrowseMailbox || _curBarLayout == Tool::BrowseSyncArea)
	{
		// Update script comment caption
		Tool::ButtonBand * band = GetButtonBand (Tool::ScriptCommentBandId);
		Assert (band != 0);
		if (_curBarLayout == Tool::BrowseMailbox)
			band->SetCaption ("Next script:");
		else
			band->SetCaption ("Current script:");
	}
	else if (_curBarLayout == Tool::NotInProject)
	{
		_dropDownCtrl->Refresh ("");
		_currentFolderDisplay.SetText ("");
		_scriptCommentDisplay.SetText ("");
		_urlDisplayCtrl->RefreshUrlList ("");
	}
	Tool::DynamicRebar::LayoutChange (Tool::BandLayoutTable [_curBarLayout]);
}

void InstrumentBar::SetFont (Font::Descriptor const & font)
{
	_dropDownCtrl->SetFont (font);
	_urlDisplayCtrl->SetFont (font);
	_currentFolderDisplay.SetFont (font);
	_checkinStatusDisplay.SetFont (font);
	_scriptCommentDisplay.SetFont (font);
}

void InstrumentBar::RefreshScriptComment (std::string const & comment)
{
	unsigned int eolPos = comment.find_first_of ("\r\n");
	if (eolPos != std::string::npos)
	{
		// Multi-line caption.	Display first line (add "..." at the end)
		// Complete caption will be displayed as tool tip.
		std::string firstLine (&comment [0], eolPos);
		firstLine += " ...";
		_scriptCommentDisplay.SetText (firstLine.c_str ());
	}
	else
	{
		_scriptCommentDisplay.SetText (comment.c_str ());
	}
	Tool::ButtonBand * band = GetButtonBand (Tool::ScriptCommentBandId);
	band->RegisterToolTip (_scriptCommentDisplay, GetParentWin (), comment);
}

void InstrumentBar::RefreshCurrentFolder (std::string const & path)
{
	_currentFolderDisplay.SetText (path.c_str ());
}

void InstrumentBar::RefreshCheckinStatus (std::string const & status)
{
	_checkinStatusDisplay.SetText (status.c_str ());
}

void InstrumentBar::RefreshFilter (std::string const & caption)
{
	_dropDownCtrl->Refresh (caption);
}

bool InstrumentBar::OnChildSize (unsigned bandIdx,
								 unsigned bandId,
								 Win::Rect & newChildRect,
								 Win::Rect const & newBandRect) throw ()
{
	dbg << "--> InstrumentBar::OnChildSize" << std::endl;
	dbg << "    Band idx: " << std::dec << bandIdx << "; band id: " << std::dec << bandId << std::endl;
	dbg << "    Child new rect: " << newChildRect << std::endl;
	dbg << "    Band new rect: " << newBandRect << std::endl;
	if (bandId == Tool::CurrentFolderBandId ||
		bandId == Tool::CheckinStatusBandId ||
		bandId == Tool::ScriptCommentBandId ||
		bandId == Tool::HistoryFilterBandId ||
		bandId == Tool::UrlBandId)
	{
		Tool::ButtonBand const * band = GetButtonBand (bandId);
		unsigned buttonExtent = band->GetButtonExtent ();
		Win::Rect rect;
		rect.left = buttonExtent + Tool::DynamicRebar::fudgeFactor;
		rect.right = newChildRect.right - newChildRect.left - Tool::DynamicRebar::fudgeFactor;
		if (bandId == Tool::HistoryFilterBandId || bandId == Tool::UrlBandId)
		{
			Win::Dow::Handle parentWin = GetParentWin ();
			Win::ClientRect parentRect (parentWin);
			rect.bottom = rect.top + parentRect.Height () - Tool::DynamicRebar::fudgeFactor/ 2;
			if (bandId == Tool::HistoryFilterBandId)
			{
				_dropDownCtrl->GetView ().Move (rect);
			}
			else
			{
				Assert (bandId == Tool::UrlBandId);
				_urlDisplayCtrl->GetView ().Move (rect);
			}
		}
		else
		{
			Assert (bandId == Tool::CurrentFolderBandId ||
					bandId == Tool::CheckinStatusBandId ||
					bandId == Tool::ScriptCommentBandId);
			rect.top += 2 * Tool::DynamicRebar::fudgeFactor;
			rect.bottom = rect.top + Tool::DefaultButtonWidth;
			if (bandId == Tool::CurrentFolderBandId)
			{
				_currentFolderDisplay.Move (rect);
			}
			else if (bandId == Tool::CheckinStatusBandId)
			{
				_checkinStatusDisplay.Move (rect);
			}
			else
			{
				Assert (bandId == Tool::ScriptCommentBandId);
				_scriptCommentDisplay.Move (rect);
			}
		}

	}
	dbg << "<-- InstrumentBar::OnChildSize" << std::endl;
	return true;
}
