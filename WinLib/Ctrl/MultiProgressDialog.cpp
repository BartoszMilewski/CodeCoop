//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "WinLibBase.h"
#include "MultiProgressDialog.h"

#include <Win/MsgLoop.h>

Progress::MultiCtrlHandler::MultiCtrlHandler (Progress::MeterDialogData & data,
											  Progress::Channel & overall,
											  Progress::Channel & specific)
	: Progress::CtrlHandler (data, Progress::MultiMeterDialog::DlgId),
	  _overall (overall),
	  _specific (specific)
{}

bool Progress::MultiCtrlHandler::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_caption.Init (dlgWin, Progress::MultiMeterDialog::CaptionId);
	_overallActivity.Init (dlgWin, Progress::MultiMeterDialog::OverallActivityId);
	_overallBar.Init (dlgWin, Progress::MultiMeterDialog::OverallProgressBarId);
	_specificActivity.Init (dlgWin, Progress::MultiMeterDialog::SpecificActivityId);
	_specificBar.Init (dlgWin, Progress::MultiMeterDialog::SpecificProgressBarId);
	_cancel.Init (dlgWin, Out::Cancel);

	dlgWin.SetText (_dlgData.GetTitle ());
	dlgWin.CenterOverOwner ();

	if (!_overall.CanCancel ())
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

bool Progress::MultiCtrlHandler::OnCancel () throw ()
{
	_overall.Cancel ();
	EndCancel ();
	return true;
}

void Progress::MultiCtrlHandler::Refresh ()
{
	// Refresh caption
	_caption.SetText (_dlgData.GetCaption ());

	int min, max, step;
	// Refresh overall meter
	if (_overall.GetRange (min, max, step))
		_overallBar.SetRange (min, max, step);
	int position;
	if (_overall.GetPosition (position))
		_overallBar.StepTo (position);
	if (_overall.GetActivity (_overallActivityStr))
		_overallActivity.SetText (_overallActivityStr);

	// Refresh specific meter
	if (_specific.GetRange (min, max, step))
		_specificBar.SetRange (min, max, step);
	if (_specific.GetPosition (position))
		_specificBar.StepTo (position);
	if (_specific.GetActivity (_specificActivityStr))
		_specificActivity.SetText (_specificActivityStr);

	Win::Dow::Handle dlgWin (GetWindow ());
	dlgWin.Update ();
}

//IDD_PROGRESS_METER DIALOGEX 0, 0, 280, 100
//STYLE DS_SETFONT | DS_MODALFRAME | DS_3DLOOK | WS_POPUP | WS_CAPTION
//FONT 8, "MS Sans Serif", 0, 0, 0x0
//BEGIN
//	DEFPUSHBUTTON   "Cancel",IDCANCEL,223,79,50,14
//	LTEXT           "Static",IDC_PROGRESS_METER_CAPTION,7,5,266,25,SS_SUNKEN
//	CONTROL         "Progress1",IDC_PROGRESS_METER_BAR,"msctls_progress32", WS_BORDER,7,81,213,11
//	LTEXT           "Static",IDC_PROGRESS_METER_ACTIVITY,7,66,266,10
//	CONTROL         "",IDC_PROGRESS_METER_BAR2,"msctls_progress32",WS_BORDER,7,50,266,11
//	LTEXT           "Static",IDC_PROGRESS_METER_ACTIVITY2,7,35,266,10
//END

void Progress::MultiMeterDialog::CreateDialogTemplate (::Dialog::Template & tmpl)
{
	::Dialog::TemplateMaker::Button cancelButton (Out::Cancel);
	cancelButton.SetRect (223, 79, 50, 14);
	cancelButton.SetText (L"Cancel");
	cancelButton.Style () << Win::Button::Style::Default;

	::Dialog::TemplateMaker::StaticText caption (CaptionId);
	caption.SetRect (7, 5, 266, 25);
	caption.Style () << Win::Static::Style::Sunken;

	::Dialog::TemplateMaker::StaticText overallActivity (OverallActivityId);
	overallActivity.SetRect (7, 35, 266, 10);

	::Dialog::TemplateMaker::ProgressBar overallBar (OverallProgressBarId);
	overallBar.SetRect (7, 50, 266, 11);

	::Dialog::TemplateMaker::StaticText specificActivity (SpecificActivityId);
	specificActivity.SetRect (7, 66, 266, 10);

	::Dialog::TemplateMaker::ProgressBar specificBar (SpecificProgressBarId);
	specificBar.SetRect (7, 81, 213, 11);

	::Dialog::TemplateMaker templateMaker;
	templateMaker.SetRect (Win::Rect (0, 0, 280, 100));

	templateMaker.AddItem (&cancelButton);
	templateMaker.AddItem (&caption);
	templateMaker.AddItem (&overallActivity);
	templateMaker.AddItem (&overallBar);
	templateMaker.AddItem (&specificActivity);
	templateMaker.AddItem (&specificBar);
	templateMaker.Create (tmpl);
}
