//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include <WinLibBase.h>
#include "Rebar.h"

#include <Win/Message.h>
#include <Win/Geom.h>

Tool::Rebar::BandInfo::BandInfo ()
{
	::ZeroMemory (this, sizeof (REBARBANDINFO));
	cbSize = sizeof (REBARBANDINFO);
}

void Tool::Rebar::BandInfo::SetCaption (std::string const & caption)
{
	_caption = caption;
	fMask |= RBBIM_TEXT;
	lpText = const_cast<LPSTR>(_caption.c_str ());
}

void Tool::Rebar::BandInfo::AddChildWin (Win::Dow::Handle win)
{
	Assert (hwndChild == 0);
	fMask |= RBBIM_CHILD;
	hwndChild = win.ToNative ();
}

void Tool::Rebar::BandInfo::InitBandSizes (unsigned int width, unsigned int height)
{
	// Child window size
	fMask |= RBBIM_CHILDSIZE;
	cxMinChild = width;
	cyMinChild = height;
	// Ideal size
	fStyle |= RBBS_USECHEVRON;	// Show chevron when smaller the ideal size
	fMask |= RBBIM_IDEALSIZE;
	cxIdeal = width;
	// Total band size
	fMask |= RBBIM_SIZE;
	cx = width;
}

void Tool::Rebar::BandInfo::SetId (unsigned id)
{
	fMask |= RBBIM_ID;
	wID = id;
}

void Tool::Rebar::Size (Win::Rect const & toolRect)
{
	Move (toolRect);
}

void Tool::Rebar::AppendBand (Tool::Rebar::BandInfo const & info)
{
	Win::Message msg (RB_INSERTBAND, -1, reinterpret_cast<LPARAM>(&info));
	SendMsg (msg);
}

void Tool::Rebar::ShowBand (unsigned int bandIdx, bool show)
{
	Win::Message msg (RB_SHOWBAND, bandIdx, show ? 1 : 0);
	SendMsg (msg);
}

bool Tool::Rebar::DeleteBand (unsigned int bandIdx)
{
	Win::Message msg (RB_DELETEBAND, 0);
	SendMsg (msg);
	return msg.GetResult () != 0;
}

void Tool::Rebar::Clear ()
{
	while (DeleteBand (0))
		continue;
}

Tool::RebarMaker::RebarMaker (Win::Dow::Handle parentWin, int id)
	: Win::ControlMaker (REBARCLASSNAME, parentWin, id)
{
	Win::CommonControlsRegistry::Instance()->Add (Win::CommonControlsRegistry::COOL);
	Win::Maker::Style () << Win::Style::Ex::ToolWindow;
}

Tool::ButtonBand::ButtonBand (Win::Dow::Handle parentWin,
							  unsigned buttonsBitmapId,
							  unsigned buttonWidth,
							  Cmd::Vector const & cmdVector,
							  Tool::Item const * buttonItems,
							  Tool::BandItem const * bandItem)
	: _id (bandItem->bandId),
	  _toolBar (parentWin,
				bandItem->bandId,
				buttonsBitmapId,
				buttonWidth,
				cmdVector,
				buttonItems,
				Win::Style (Tool::Style::NoResize |
							Tool::Style::NoParentAlign |
							Tool::Style::NoDivider |
							Tool::Style::Flat |
							Tool::Style::Tips))
{
	_toolBar.SetLayout (bandItem->buttonLayout);
	_toolBar.AutoSize ();
	_defaultToolTipDelay = _toolBar.GetToolTipDelay ();
	AddChildWin (_toolBar);
	SetId (bandItem->bandId);
	unsigned barWidth = 0;
	unsigned barHeight = 0;
	_toolBar.GetMaxSize (barWidth, barHeight);
	// Set the height to the default bar height, so other
	// controls placed on the tool bar will fit nicely
	InitBandSizes (barWidth + bandItem->extraSpace, Tool::DefaultBarHeight);
}

unsigned Tool::ButtonBand::GetButtonExtent () const
{
	unsigned width = 0;
	unsigned height = 0;
	_toolBar.GetMaxSize (width, height);
	return width;
}

void Tool::ButtonBand::RegisterToolTip (Win::Dow::Handle registeredWin,
										Win::Dow::Handle winNotify,
										std::string const & tip)
{
	ToolTipMap::const_iterator iter = _registeredToolTips.find (registeredWin);
	if (iter == _registeredToolTips.end ())
		_toolBar.AddWindow (registeredWin, winNotify);
	
	_registeredToolTips [registeredWin] = tip;
}

bool Tool::ButtonBand::FillToolTip (Tool::TipForCtrl * tip) const
{
	if (_toolBar.IsCmdButton (tip->IdFrom ()))
	{
		_toolBar.FillToolTip (tip);
		CalculateToolTipDelay (tip->GetTipText ());
		return true;
	}
	return false;
}

bool Tool::ButtonBand::FillToolTip (Tool::TipForWindow * tip) const
{
	ToolTipMap::const_iterator iter = _registeredToolTips.find (tip->WinFrom ());
	if (iter != _registeredToolTips.end ())
	{
		tip->SetText (iter->second.c_str ());
		CalculateToolTipDelay (tip->GetTipText ());
		return true;
	}
	return false;
}

void Tool::ButtonBand::CalculateToolTipDelay (char const * tip) const
{
	// Short tip (less then 40 characters) gets default display time.
	// Long tip is displayed for 20 seconds.
	if (strlen (tip) < 40)
		_toolBar.SetToolTipDelay (_defaultToolTipDelay);
	else
		_toolBar.SetToolTipDelay (20000);
}

bool Notify::RebarHandler::OnNotify (NMHDR * hdr, long & result)
{
	// hdr->code
	// hdr->idFrom;
	// hdr->hwndFrom;
	switch (hdr->code)
	{
	case RBN_AUTOSIZE:
		{
			LPNMRBAUTOSIZE autoSizeInfo = reinterpret_cast<LPNMRBAUTOSIZE>(hdr);
			return OnAutoSize (autoSizeInfo->rcTarget,
							   autoSizeInfo->rcActual,
							   autoSizeInfo->fChanged != 0);
		}
	case RBN_CHEVRONPUSHED:
		{
			LPNMREBARCHEVRON chevronInfo = reinterpret_cast<LPNMREBARCHEVRON>(hdr);
			return OnChevronPushed (chevronInfo->uBand,
									chevronInfo->wID,
									chevronInfo->lParam,
									chevronInfo->rc,
									chevronInfo->lParamNM);
		}
	case RBN_CHILDSIZE:
		{
			LPNMREBARCHILDSIZE childSizeInfo = reinterpret_cast<LPNMREBARCHILDSIZE>(hdr);
			Win::Rect childRect (childSizeInfo->rcChild);
			if (OnChildSize (childSizeInfo->uBand,
							 childSizeInfo->wID,
							 childRect,
							 childSizeInfo->rcBand))
			{
				childSizeInfo->rcChild = childRect;
				return true;
			}
		}
	case RBN_HEIGHTCHANGE:
		return OnHeightChange ();
	case RBN_LAYOUTCHANGED:
		return OnLayoutChange ();
	}
	return false;
}
