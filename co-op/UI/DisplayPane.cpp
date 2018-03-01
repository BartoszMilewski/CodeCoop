//------------------------------------
//  (c) Reliable Software, 2005 - 2006
//------------------------------------

#include "precompiled.h"
#include "DisplayPane.h"
#include "WidgetBrowser.h"
#include "Controllers.h"
#include "WikiController.h"
#include "DynamicRebar.h"

#include <Ctrl/FocusBarWin.h>

//
// Bar
//

unsigned Pane::Bar::GetHeight () const
{
	Win::ClientRect rect (_win);
	return rect.Height ();
}

//
// Display Pane Focus Bar
//

Pane::FocusBar::FocusBar (::FocusBar::Ctrl * ctrl)
	: Bar (ctrl->GetWindow ()),
	  _ctrl (ctrl)
{}

unsigned Pane::FocusBar::GetHeight () const
{
	Assert (_ctrl != 0);
	return _ctrl->GetHeight ();
}

void Pane::FocusBar::Refresh (std::string const & text, bool isActive)
{
	Assert (_ctrl != 0);
	if (isActive)
		_ctrl->TurnOn ();
	else
		_ctrl->TurnOff ();
}

//
// Display Pane Focus Bar
//

Pane::ToolBar::ToolBar (Tool::DynamicRebar & rebar)
	: Bar (rebar.GetWindow ()),
	  _rebar (rebar)
{}

unsigned Pane::ToolBar::GetHeight () const
{
	return _rebar.Height ();
}

void Pane::ToolBar::Refresh (std::string const & text, bool isActive)
{
	_rebar.RefreshTextField (text);
	_rebar.RefreshButtons ();
	if (isActive)
		_rebar.Enable ();
	else
		_rebar.Disable ();
}

//
// Display Pane
//

void Pane::ShowBar ()
{
	if (_bar.get () != 0)
		_bar->Show ();
}

void Pane::HideBar ()
{
	if (_bar.get () != 0)
		_bar->Hide ();
}

void Pane::MoveBar (Win::Rect & paneRect)
{
	if (_bar.get () == 0)
		return;

	unsigned barHeight = _bar->GetHeight ();
	if (paneRect.Height () < 2 * barHeight)
	{
		// Pane rectangle too small to shown this bar window
		_bar->Hide ();
	}
	else
	{
		_bar->Move (paneRect.Left (), paneRect.Top (), paneRect.Width (), barHeight);
		// Reduce pane rectangle height
		paneRect.top += barHeight;
		_bar->Show ();
	}
}

void Pane::OnFocus (unsigned focusId)
{
	RefreshBar (focusId, false);	// Not forced refresh
	if (focusId == _id)
	{
		// Pane gets keyboard focus
		Assert (_browser.get () != 0);
		_browser->OnFocus ();
	}
}

void Pane::RefreshBar (unsigned focusId, bool force)
{
	if (_bar.get () == 0)
		return;

	Assert (_browser.get () != 0);
	_bar->Refresh (_browser->GetCaption (), (focusId == _id) || force);
}

RecordSet const * Pane::GetRecordSet() const
{
	Assert (_browser.get () != 0);
	return _browser->GetRecordSet ();
}

void Pane::VerifyRecordSet() const
{
	RecordSet const * recordSet = _browser->GetRecordSet ();
	// Revisit: Browser page doesn't have a record set yet
	if (recordSet != 0 && recordSet->IsValid ())
		recordSet->Verify();
}

bool Pane::HasTable(Table::Id tableId) const
{
	Assert (_browser.get () != 0);
	RecordSet const * rs = _browser->GetRecordSet ();
	if (rs != 0)
		return rs->GetTableId() == tableId;
	return false;
}

void Pane::Attach (Observer * observer, std::string const & topic)
{
	Assert (_browser.get () != 0);
	_browser->Attach (observer, topic);
}

void Pane::Detach (Observer * observer)
{
	Assert (_browser.get () != 0);
	_browser->Detach (observer);
}

Observer * Pane::GetObserver ()
{
	return _browser.get ();
}

void Pane::SetRestrictionFlag (std::string const & name, bool val)
{
	Assert (_browser.get () != 0);
	_browser->SetRestrictionFlag (name, val);
}

void Pane::Clear (bool forGood)
{
	Assert (_browser.get () != 0);
	_browser->Clear (forGood);
}

void Pane::Invalidate ()
{
	Assert (_browser.get () != 0);
	_browser->Invalidate ();
}

//
// Table Pane
//

TablePane::TablePane (std::unique_ptr<TableController> ctrl,
					  std::unique_ptr<WidgetBrowser> browser)
	: Pane (ctrl->GetId (), std::move(browser)),
	  _ctrl (std::move(ctrl))
{}

Notify::Handler * TablePane::GetNotifyHandler () const
{
	Assert (_ctrl.get () != 0);
	return _ctrl.get ();
} 

bool TablePane::HasWindow (Win::Dow::Handle h) const
{
	Assert (_ctrl.get () != 0);
	return _ctrl->GetView () == h;
}

void TablePane::Activate (FeedbackManager & feedback)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	ShowBar ();
	_browser->Show (feedback);
	_ctrl->ShowView ();
}

void TablePane::DeActivate ()
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	HideBar ();
	_browser->Hide ();
	_ctrl->HideView ();
}

void TablePane::Move (Win::Rect const & rect)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	Win::Rect paneRect = rect;
	MoveBar (paneRect);
	_ctrl->MoveView (paneRect);
	_ctrl->ShowView ();
}

void TablePane::OnDoubleClick ()
{
	_ctrl->OnDblClick ();
}

void TablePane::InPlaceEdit (int row)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	_ctrl->SetNewItemCreation (false);
	_browser->InPlaceEdit (row);
}

void TablePane::NewItemCreation ()
{
	// New item is added to the display pane
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	_ctrl->SetNewItemCreation (true);
	_browser->BeginNewItemEdit ();
}

void TablePane::AbortNewItemCreation ()
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	_ctrl->SetNewItemCreation (false);
	_browser->AbortNewItemEdit ();
}

//
// Tree Pane
//

TreePane::TreePane (std::unique_ptr<HierarchyController> ctrl,
					std::unique_ptr<WidgetBrowser> browser)
	: Pane (ctrl->GetId (), std::move(browser)),
	  _ctrl (std::move(ctrl))
{}

bool TreePane::HasWindow (Win::Dow::Handle h) const
{
	Assert (_ctrl.get () != 0);
	return _ctrl->GetView () == h;
}

Notify::Handler * TreePane::GetNotifyHandler () const
{
	Assert (_ctrl.get () != 0);
	return _ctrl.get ();
}

void TreePane::Activate (FeedbackManager & feedback)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	ShowBar ();
	_browser->Show (feedback);
	_ctrl->ShowView ();
}

void TreePane::DeActivate ()
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	HideBar ();
	_browser->Hide ();
	_ctrl->HideView ();
}

void TreePane::Move (Win::Rect const & rect)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	Win::Rect paneRect = rect;
	MoveBar (paneRect);
	_ctrl->MoveView (paneRect);
	_ctrl->ShowView ();
}

//------------
// BrowserPane
//------------
BrowserPane::BrowserPane (std::unique_ptr<WikiBrowserController> ctrl, 
						  std::unique_ptr<WidgetBrowser> browser)
	: Pane (ctrl->GetId (), std::move(browser)),
	  _ctrl (std::move(ctrl))
{}

void BrowserPane::Move (Win::Rect const & paneRect)
{
	_ctrl->MoveView (paneRect);
}

void BrowserPane::Activate (FeedbackManager & feedback)
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	if (_browser->Show (feedback))
		_ctrl->StartNavigation (feedback);
	_ctrl->ShowView ();
}

void BrowserPane::DeActivate ()
{
	Assert (_ctrl.get () != 0);
	Assert (_browser.get () != 0);
	_browser->Hide ();
	_ctrl->HideView ();
}

void BrowserPane::Navigate (std::string const & target, int scrollPos)
{
	_ctrl->Navigate (target, scrollPos);
}