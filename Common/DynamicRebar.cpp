//------------------------------------
//  (c) Reliable Software, 2006
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "DynamicRebar.h"
#include "Resource.h"
#include "CommonUserMsg.h"

#include <Win/Message.h>
#include <Dbg/Out.h>

Tool::DynamicRebar::DynamicRebar (Win::Dow::Handle parentWin,
							Cmd::Executor & executor,
							Tool::Item const * buttonItems,
							unsigned toolBoxId,
							unsigned buttonCtrlId)
	: Notify::RebarHandler (toolBoxId),
	  _buttonItems (buttonItems),
	  _buttonCtrl (buttonCtrlId, executor),
	  _height (Tool::DefaultBarHeight)
{
	Tool::RebarMaker rebarMaker (parentWin, toolBoxId);
	rebarMaker.Style () << Win::Style (Tool::Rebar::Style::AutoSize |
									   Tool::Rebar::Style::FixedOrder |
									   Tool::Style::NoParentAlign |
									   Tool::Style::NoMoveY |
									   Tool::Rebar::Style::ToolTips);
	_rebar = rebarMaker.Create ();
}

Tool::DynamicRebar::~DynamicRebar ()
{
	_rebar.Clear ();
}

Tool::ButtonBand * Tool::DynamicRebar::AddButtonBand (Tool::BandItem const & bandItem,
												   Cmd::Vector & cmdVector)
{
	std::unique_ptr<Tool::ButtonBand> toolBand;
	toolBand.reset (new Tool::ButtonBand (_rebar,
										  IDB_BUTTONS,	// Button bitmap resource id
										  Tool::DefaultButtonWidth,
										  cmdVector,
										  _buttonItems,
										  &bandItem));
	Tool::ButtonBand * newToolBand = toolBand.get ();
	_toolBandMap [bandItem.bandId] = newToolBand;
	_myToolBands.push_back (std::move(toolBand));
	return newToolBand;
}

Control::Handler * Tool::DynamicRebar::GetControlHandler (Win::Dow::Handle winFrom, 
											   unsigned idFrom) throw ()
{
	if (_buttonCtrl.IsCmdButton (idFrom))
		return &_buttonCtrl;

	return 0;
}

void Tool::DynamicRebar::Size (Win::Rect const & toolRect)
{
	_rebar.Size (toolRect);
}

void Tool::DynamicRebar::LayoutChange (unsigned int const * bandIds)
{
	_rebar.Clear ();
	_buttonCtrl.ClearButtonIds ();
	_currentTools.clear ();
	for (unsigned i = 0; bandIds [i] != Tool::InvalidBandId; ++i)
	{
		unsigned bandId = bandIds [i];
		Tool::ButtonBand * band = _toolBandMap [bandId];
		Assert (band != 0);
		_rebar.AppendBand (*band);
		_buttonCtrl.AddButtonIds (band->begin (), band->end ());
		_currentTools.push_back (band);
	}
}

void Tool::DynamicRebar::Disable ()
{
    std::for_each (_currentTools.begin (), _currentTools.end (), [](Tool::ButtonBand * band)
    {
        band->Disable(); 
    });
}

void Tool::DynamicRebar::Enable ()
{
	std::for_each (_currentTools.begin (), _currentTools.end (),  [](Tool::ButtonBand * band)
    {
        band->Enable(); 
    });
}

void Tool::DynamicRebar::RefreshButtons ()
{
	std::for_each (_currentTools.begin (), _currentTools.end (),  [](Tool::ButtonBand * band)
    {
        band->RefreshButtons(); 
    });
}

// Returns true when tool tip filled successfully
bool Tool::DynamicRebar::FillToolTip (Tool::TipForCtrl * tip) const
{ 
	for (ToolBandSeq seq = _currentTools.begin (); seq != _currentTools.end (); ++seq)
	{
		Tool::ButtonBand const * band = *seq;
		if (band->FillToolTip (tip))
			return true;
	}
	return false;
}

// Returns true when tool tip filled successfully
bool Tool::DynamicRebar::FillToolTip (Tool::TipForWindow * tip) const
{ 
	for (ToolBandSeq seq = _currentTools.begin (); seq != _currentTools.end (); ++seq)
	{
		Tool::ButtonBand const * band = *seq;
		if (band->FillToolTip (tip))
			return true;
	}
	return false;
}

bool Tool::DynamicRebar::OnChevronPushed (unsigned bandIdx,
									   unsigned bandId,
									   unsigned appParam,
									   Win::Rect const & chevronRect,
									   unsigned notificationParam) throw ()
{
	// Revisit:
	//	1. Create a popup window 
	//	2. Create a tool bar with the popup as the parent and add the hidden buttons to it 
	//	3. Show the popup window just below chevrons 
	return false;
}

bool Tool::DynamicRebar::OnHeightChange () throw ()
{
	// If height change then notify parent about layout change
	Win::ClientRect rect (_rebar);
	if (_height != rect.Height ())
	{
		_height = rect.Height ();
		Win::Dow::Handle parent = _rebar.GetParent ();
		Win::UserMessage layoutChange (UM_LAYOUT_CHANGE);
		parent.PostMsg (layoutChange);
	}
	return true;
}

//--------------------
// Tool Tip Handler
//--------------------

Tool::DynamicRebar::TipHandler::TipHandler (unsigned id, Win::Dow::Handle win)
	: Notify::ToolTipHandler (id)
{}

bool Tool::DynamicRebar::TipHandler::OnNeedText (Tool::TipForWindow * tip) throw ()
{
	for (RebarList::const_iterator iter = _rebars.begin ();
		 iter != _rebars.end ();
		 ++iter)
	{
		Tool::DynamicRebar const * rebar = *iter;
		if (rebar->FillToolTip (tip))
			break;
	}
	return true;
}

bool Tool::DynamicRebar::TipHandler::OnNeedText (Tool::TipForCtrl * tip) throw ()
{
	for (RebarList::const_iterator iter = _rebars.begin ();
		 iter != _rebars.end ();
		 ++iter)
	{
		Tool::DynamicRebar const * rebar = *iter;
		if (rebar->FillToolTip (tip))
			break;
	}
	return true;
}
