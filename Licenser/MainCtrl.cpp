// Reliable Software (c) 2005
#include "precompiled.h"
#include "MainCtrl.h"
#include "Resource/resource.h"
#include "OutSink.h"
#include <Win/Metrics.h>
#include <Win/Notification.h>

MainController::MainController (Win::MessagePrepro & prepro)
	: Dialog::ModelessController (prepro,ID_MAIN),
	  _prepro (prepro),
	  Notify::TabHandler (ID_TAB),
	  _initMsg ("InitMessage"),
	  _dlgCtrl1 (_request, _model.GetWorkFolder ()),
	  _dlgCtrl2 (_request),
	  _dlgCtrl3 (_request),
	  _curCtrlHandler (0)
{}

MainController::~MainController ()
{}

void MainController::Initialize () throw (Win::Exception)
{
	// Attach icon to main dialog window
	Win::Dow::Handle win = GetWindow ();
	Icon::StdMaker iconMaker;
	_icon = iconMaker.Load (GetWindow ().GetInstance (), DLG_ICON);
	win.SetIcon (_icon);

	// Create tab-control child window
	_tabs.reset (new TabCtrl (win, ID_TAB));
	TabCtrl::View & tabView = _tabs->GetView ();
	tabView.SetFont (8, "Tahoma");
	// Add tab items
	tabView.InsertPageTab (LicReseller, "Reseller");
	tabView.InsertPageTab (LicDistribution, "Distribution");
	tabView.InsertPageTab (LicRegular, "Regular");
	tabView.SetSelection (LicRegular);

	// Load dialog template from resources
	_dlgTmp1.Load (win.GetInstance (), IDD_SINGLE);

	// Create temporary dialog to get its rectangle
	Dialog::ModelessMaker dlgMaker1 (_dlgCtrl1, _prepro, win);
	Dialog::Handle dlg = dlgMaker1.Create (tabView.ToWin (), _dlgTmp1);
	// In dlg template units
	Win::Rect dlgRect1 (0, 0, _dlgTmp1.Width (), _dlgTmp1.Height ());
	dlg.MapRectangle (dlgRect1); // result is in pixels
	dlg.Destroy ();

	_dlgTmp2.Load (win.GetInstance (), IDD_DISTRIB);

	Dialog::ModelessMaker dlgMaker2 (_dlgCtrl2, _prepro, GetWindow ());
	dlg = dlgMaker2.Create (tabView.ToWin (), _dlgTmp2);
	Win::Rect dlgRect2 (0, 0, _dlgTmp2.Width (), _dlgTmp2.Height ());
	dlg.MapRectangle (dlgRect2); // result in pixels
	dlg.Destroy ();

	_dlgTmp3.Load (win.GetInstance (), IDD_BLOCK);

	Dialog::ModelessMaker dlgMaker3 (_dlgCtrl3, _prepro, GetWindow ());
	dlg = dlgMaker3.Create (tabView.ToWin (), _dlgTmp3);
	Win::Rect dlgRect3 (0, 0, _dlgTmp3.Width (), _dlgTmp3.Height ());
	dlg.MapRectangle (dlgRect3); // result in pixels
	dlg.Destroy ();

	// In pixels: maximum rectangle
	Win::Rect tabRect;
	tabRect.Maximize (dlgRect1.Width (), dlgRect1.Height ());
	tabRect.Maximize (dlgRect2.Width (), dlgRect2.Height ());
	tabRect.Maximize (dlgRect3.Width (), dlgRect3.Height ());

    // Calculate how large to make the tab control, so 
    // the display area can accommodate all the child dialog boxes. 
	tabView.AdjustRectangleUp (tabRect);
	int cxMargin = Dialog::BaseUnitX () / 4; 
	int cyMargin = Dialog::BaseUnitY () / 8; 

	// Note: after adjusting, the rectangle offset was negative
	tabRect.ShiftBy (cxMargin - tabRect.Left (), cyMargin - tabRect.Top ());

    // Calculate the display rectangle. 
	_dispRect = tabRect;
	tabView.AdjustRectangleDown (_dispRect);
 
	// Position and size the tab control
	tabView.Position (tabRect);

	// Move the buttons
	Win::Button button1 (win, IDOK);
	button1.ShiftTo (tabRect.left, tabRect.bottom + cyMargin);
	Win::Rect buttonRect;
	button1.GetWindowRect (buttonRect);

	Win::Button button2 (win, IDCANCEL);
	button2.ShiftTo (tabRect.Right () - buttonRect.Width (), 
					 tabRect.bottom + cyMargin);
    // Size the dialog box.
	win.Size (tabRect.Width () + 2 * cyMargin + 2 * Dialog::FrameThickX (), 
        tabRect.Height () + buttonRect.Height () + 3 * cyMargin + 
		2 * Dialog::FrameThickY () + Dialog::CaptionHeight ());

	_prepro.SetDialogFilter (win);
	// Display a dialog
	win.PostMsg (_initMsg);
}

Notify::Handler * MainController::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (IsHandlerFor (idFrom))
		return this;
	return 0;
}

bool MainController::OnRegisteredMessage (Win::Message & msg) throw ()
{
	if (msg == _initMsg)
	{
		OnSelChange ();
		return true;
	}
	return false;
}

bool MainController::OnSelChange () throw ()
{
	if (_curCtrlHandler != 0)
	{
		_curCtrlHandler->OnApplyMsg ();
		_curCtrlHandler->EndOk ();
	}
	
	TabCtrl::View & tabView = _tabs->GetView ();
	int id = tabView.GetCurSelection ();
	
	switch (id)
	{
	case 0:
		{
			_curCtrlHandler = &_dlgCtrl1;
			Dialog::ModelessMaker dlgMaker1 (_dlgCtrl1, _prepro, GetWindow ());
			_curDialog = dlgMaker1.Create (tabView.ToWin (), _dlgTmp1);
			break;
		}
	case 1:
		{
			_curCtrlHandler = &_dlgCtrl2;
			Dialog::ModelessMaker dlgMaker2 (_dlgCtrl2, _prepro, GetWindow ());
			_curDialog = dlgMaker2.Create (tabView.ToWin (), _dlgTmp2);
			break;
		}
	case 2:
		{
			_curCtrlHandler = &_dlgCtrl3;
			Dialog::ModelessMaker dlgMaker3 (_dlgCtrl3, _prepro, GetWindow ());
			_curDialog = dlgMaker3.Create (tabView.ToWin (), _dlgTmp3);
			break;
		}
	default:
		return false;
	}
	_curDialog.ShiftTo (_dispRect.left, _dispRect.top);
	_curDialog.Show ();
	return true;
}

bool MainController::OnApply () throw ()
{
	try
	{
		if (_curCtrlHandler)
			_curCtrlHandler->OnApplyMsg ();
		std::string comment;
		if (_model.PostRequest (_request))
		{
			_model.CreateLicense ();
			comment = _request.Comment ();
			_request.Clear ();
		}
		else
		{
			comment = _request.Comment ();
		}
		ClearDialog ();
		TheOutput.Display (comment.c_str ());
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	return true;
}

void MainController::ClearDialog ()
{
	if (_curCtrlHandler)
		_curCtrlHandler->EndOk ();
	_curDialog.Reset ();
	_curCtrlHandler = 0;
	OnSelChange ();
}

bool MainControlHandler::OnApply () throw ()
{
	return _ctrl->OnApply ();
}

