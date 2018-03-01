//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include "precompiled.h"
#include "CheckInDlg.h"
#include "resource.h"
#include "Prompter.h"
#include "Outputsink.h"

CheckInCommentCtrl::CheckInCommentCtrl (CheckInData * data)
	: Dialog::ControlHandler (IDD_CHECKIN),
	  _dlgData (data)
{}

// command line
// -all_checkin comment:"Comment string"
bool CheckInCommentCtrl::GetDataFrom (NamedValues const & source)
{
	TrimmedString comment;
	comment.Assign (source.GetValue ("comment"));
	_dlgData->SetComment (comment);
	return !comment.empty ();
}

bool CheckInCommentCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle win = GetWindow();
	// Offset the dialog
	Win::Rect rect;
	win.GetWindowRect(rect);
	rect.ShiftBy(100, 80);
	win.Move(rect);

	_okButton.Init (win, Out::OK);
	_lastComment.Init (win, IDC_CHECKIN_LAST_COMMENT);
	_keepCheckedOut.Init (win, IDC_CHECKIN_KEEPCHECKEDOUT);
	_comment.Init (win, IDC_CHECKIN_COMMENT);
	_noTest.Init (win, IDC_RADIO1);
	_someTest.Init (win, IDC_RADIO2);
	_thoroughTest.Init (win, IDC_RADIO3);
	_notApplicableTest.Init (win, IDC_RADIO4);
	_notApplicableTest.Check ();

	if (_dlgData->GetLastRejectedComment ().empty ())
		_lastComment.Disable ();
	if (_dlgData->IsKeepCheckedOut ())
		_keepCheckedOut.Check ();
	else
		_keepCheckedOut.UnCheck ();
	_okButton.Disable ();
	return true;
}

bool CheckInCommentCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_CHECKIN_COMMENT:
		if (_comment.IsChanged (notifyCode))
		{
			if (_comment.GetLen () != 0)
				_okButton.Enable ();
			else
				_okButton.Disable ();
			return true;
		}
		break;
	case IDC_CHECKIN_LAST_COMMENT:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_lastComment.IsChecked ())
			{
				_dlgData->UseLastRejectedComment ();
				_comment.SetString (_dlgData->GetLastRejectedComment ().c_str ());
				_okButton.Enable ();
			}
			else
			{
				_dlgData->ClearComment ();
				_comment.Clear ();
				_okButton.Disable ();
			}
			return true;
		}
		break;
	}
    return false;
}

bool CheckInCommentCtrl::OnApply () throw ()
{
	TrimmedString comment (_comment.GetString ());
	if (comment.empty ())
	{
		TheOutput.Display ("Please provide a check-in comment");
	}
	else
	{
		if (_noTest.IsChecked ())
			comment += " (untested)";
		else if (_someTest.IsChecked ())
			comment += " (somewhat tested)";
		else if (_thoroughTest.IsChecked ())
			comment += " (well tested)";
		// default: nothing, if N/A
		_dlgData->SetComment (comment);
		_dlgData->SetKeepCheckedOut (_keepCheckedOut.IsChecked ());
		EndOk ();
	}
	return true;
}

CheckInNoChangesCtrl::CheckInNoChangesCtrl (CheckInData * data)
	: Dialog::ControlHandler (IDD_CHECKIN_NO_CHANGES),
	  _dlgData (data)
{}

bool CheckInNoChangesCtrl::OnInitDialog () throw (Win::Exception)
{
	_keepCheckedOut.Init (GetWindow (), IDC_KEEP_CHECKED_OUT);
	if (_dlgData->IsKeepCheckedOut ())
		_keepCheckedOut.Check ();
	else
		_keepCheckedOut.UnCheck ();
	return true;
}

bool CheckInNoChangesCtrl::OnApply () throw ()
{
	_dlgData->SetKeepCheckedOut (_keepCheckedOut.IsChecked ());
	EndOk ();
	return true;
}

bool CheckInUI::Query (std::string const & lastRejectedComment, bool keepCheckedOut)
{
	_dlgData.SetLastRejectedComment (lastRejectedComment);
	_dlgData.SetKeepCheckedOut (keepCheckedOut);
	CheckInCommentCtrl ctrl (&_dlgData);
    return ThePrompter.GetData (ctrl, _source);
}

void CheckInUI::NoChangesDetected (bool keepCheckedOut)
{
	_dlgData.SetKeepCheckedOut (keepCheckedOut);
	CheckInNoChangesCtrl ctrl (&_dlgData);
	if (!ThePrompter.GetData (ctrl, _source))
		_dlgData.SetKeepCheckedOut (true); // don't uncheckout on CANCEL
}
