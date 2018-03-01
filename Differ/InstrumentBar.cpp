//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "InstrumentBar.h"
#include "ButtonTable.h"
#include "UiDifferIds.h"

#include <Dbg/Out.h>

InstrumentBar::InstrumentBar (Win::Dow::Handle parentWin,
							  Cmd::Vector & cmdVector,
							  Cmd::Executor & executor)
	: Tool::DynamicRebar (parentWin, executor, Tool::Buttons, DIFFER_TOOLBOX_ID, DIFFER_TOOLBOX_CTRL_ID)
{
	CreateToolBands (cmdVector);
	// Add non-button controls to the selected button bands
	CreateDisplayTools (parentWin);
	LayoutChange (Tool::BandLayoutTable [Tool::DifferButtons]);
}

void InstrumentBar::CreateToolBands (Cmd::Vector & cmdVector)
{
	// Create all button bands
	for (unsigned i = 0; Tool::Bands [i].bandId != Tool::InvalidBandId; ++i)
	{
		AddButtonBand (Tool::Bands [i], cmdVector);
	}
}

void InstrumentBar::CreateDisplayTools (Win::Dow::Handle parentWin)
{
	Tool::ButtonBand * band = GetButtonBand (Tool::FileDetailsBandId);
	Assert (band != 0);
	{
		Win::InfoDisplayMaker displayMaker (parentWin, FILE_DETAILS_DISPLAY_ID);
		_fileDatailsDisplay.Reset (displayMaker.Create (band->GetWindow ()));
		_fileDatailsDisplay.Show ();
	}
	band->SetCaption ("Current file:");
	band->RegisterToolTip (_fileDatailsDisplay, parentWin, "");
}

Control::Handler * InstrumentBar::GetControlHandler (Win::Dow::Handle winFrom, 
													 unsigned idFrom) throw ()
{
	return Tool::DynamicRebar::GetControlHandler (winFrom, idFrom);
}

void InstrumentBar::SetFont (Font::Descriptor const & font)
{
	_fileDatailsDisplay.SetFont (font);
}

void InstrumentBar::RefreshFileDetails (std::string const & summary, std::string const & fullInfo)
{
	_shortFileInfo = summary;
	_fileDatailsDisplay.SetText (_shortFileInfo.c_str ());
	Tool::ButtonBand * band = GetButtonBand (Tool::FileDetailsBandId);
	band->RegisterToolTip (_fileDatailsDisplay, GetParentWin (), fullInfo);
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
	if (bandId == Tool::FileDetailsBandId)
	{
		Tool::ButtonBand const * band = GetButtonBand (bandId);
		unsigned buttonExtent = band->GetButtonExtent ();
		Win::Rect rect;
		rect.left = buttonExtent + Tool::DynamicRebar::fudgeFactor;
		rect.right = newChildRect.right - newChildRect.left - Tool::DynamicRebar::fudgeFactor;
		rect.top += Tool::DynamicRebar::fudgeFactor;
		rect.bottom = rect.top + Tool::DefaultButtonWidth;
		_fileDatailsDisplay.Move (rect);
	}
	dbg << "<-- InstrumentBar::OnChildSize" << std::endl;
	return true;
}
