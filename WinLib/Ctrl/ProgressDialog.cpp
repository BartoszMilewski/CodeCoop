//------------------------------------
//  (c) Reliable Software, 1998 - 2007
//------------------------------------

#include "WinLibBase.h"
#include "ProgressDialog.h"

#include <Win/MsgLoop.h>

void Progress::Dialog::OnSetRange ()
{
	if (_dlgHandle.IsNull ())
	{
		// Create progress meter dialog
		if (_parentWin.IsNull ())
		{
			// Showing progress meter on the desktop -- don't delay
			// progress meter dialog display
			_dlgData.SetInitialDelay (0);
		}
		std::unique_ptr<::Dialog::ModelessController> ctrl
			(new Progress::DialogController (*GetControlHandler (),
											 _msgPrepro,
											 _dlgId));
		::Dialog::ModelessMaker progressMeter (*GetControlHandler (), std::move(ctrl));
		::Dialog::Template dlgTemplate;
		CreateDialogTemplate (dlgTemplate);
		_dlgHandle = progressMeter.Create (_parentWin, dlgTemplate);
	}
	else
	{
		// Progress meter dialog already created
		Progress::CtrlHandler * ctrlHandler = GetControlHandler ();
		if (ctrlHandler != 0)
			ctrlHandler->Refresh ();
	}
}

void Progress::Dialog::PumpDialog ()
{
	_msgPrepro.PumpDialog ();
}

void Progress::Dialog::Close ()
{
	if (!_dlgHandle.IsNull ())
	{
		_dlgHandle.Destroy ();
		_dlgHandle.Reset ();
	}
}

Progress::BlindDialog::BlindDialog (Win::MessagePrepro & msgPrepro)
	: Progress::Dialog ("", Win::Dow::Handle (), msgPrepro, 0, 0)
{
	_blindMeter.reset (new Progress::Meter ());
}


Progress::SingleCtrlHandler::SingleCtrlHandler (Progress::MeterDialogData & data,
												Progress::Channel & channel)
	: Progress::CtrlHandler (data, Progress::MeterDialog::DlgId),
	  _channel (channel)
{
}

bool Progress::SingleCtrlHandler::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_caption.Init (dlgWin, Progress::MeterDialog::CaptionId);
	_activity.Init (dlgWin, Progress::MeterDialog::ActivityId);
	_bar.Init (dlgWin, Progress::MeterDialog::ProgressBarId);
	_cancel.Init (dlgWin, Out::Cancel);

	dlgWin.SetText (_dlgData.GetTitle ());
	dlgWin.CenterOverOwner ();

	if (!_channel.CanCancel ())
		_cancel.Disable ();

	_progressTimer.Attach (dlgWin);
	unsigned int initialDelay = _dlgData.GetInitialDelay ();
	if (initialDelay != 0)
	{
		// Wait initial delay milliseconds before showing progress meter
		_progressTimer.Set (initialDelay);
	}
	else
	{
		_progressTimer.Set (Progress::CtrlHandler::Tick);	// Progress tick is 0,5 second
		Show ();
	}

	return true;
}

bool Progress::SingleCtrlHandler::OnCancel () throw ()
{
	_channel.Cancel ();
	EndCancel ();
	return true;
}

void Progress::SingleCtrlHandler::Refresh ()
{
	_caption.SetText (_dlgData.GetCaption ());

	int min, max, step;
	if (_channel.GetRange (min, max, step))
		_bar.SetRange (min, max, step);
	int position;
	if (_channel.GetPosition (position))
		_bar.StepTo (position);
	if (_channel.GetActivity (_activityStr))
		_activity.SetText (_activityStr);

	Win::Dow::Handle dlgWin (GetWindow ());
	dlgWin.Update ();
}

// IDD_PROGRESS_METER DIALOG  0, 0, 280, 66
// STYLE DS_SETFONT | DS_MODALFRAME | DS_3DLOOK | WS_POPUP | WS_CAPTION
// FONT 8, "MS Sans Serif"
// BEGIN
// DEFPUSHBUTTON   "Cancel",IDCANCEL,223,45,50,14
// LTEXT           "Static",IDC_PROGRESS_METER_CAPTION,7,4,266,25,SS_SUNKEN
// CONTROL         "Progress1",IDC_PROGRESS_METER_BAR,"msctls_progress32",WS_BORDER,7,47,213,11
// LTEXT           "Static",IDC_PROGRESS_METER_ACTIVITY,7,32,266,10
// END

void Progress::MeterDialog::CreateDialogTemplate (::Dialog::Template & tmpl)
{
	::Dialog::TemplateMaker::Button cancelButton (Out::Cancel);
	cancelButton.SetRect (223, 45, 50, 14);
	cancelButton.SetText (L"Cancel");
	cancelButton.Style () << Win::Button::Style::Default;

	::Dialog::TemplateMaker::StaticText caption (CaptionId);
	caption.SetRect (7, 4, 266, 25);
	caption.Style () << Win::Static::Style::Sunken;

	::Dialog::TemplateMaker::StaticText activity (ActivityId);
	activity.SetRect (7, 32, 266, 10);

	::Dialog::TemplateMaker::ProgressBar bar (ProgressBarId);
	bar.SetRect (7, 47, 213, 11);

	::Dialog::TemplateMaker templateMaker;
	templateMaker.SetRect (Win::Rect (0, 0, 280, 66));

	templateMaker.AddItem (&cancelButton);
	templateMaker.AddItem (&caption);
	templateMaker.AddItem (&activity);
	templateMaker.AddItem (&bar);
	templateMaker.Create (tmpl);
}
