//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "precompiled.h"
#include "ReceiverJoinDlg.h"
#include "MemberInfo.h"
#include "Resource.h"
#include "Catalog.h"
#include "OutputSink.h"

ReceiverJoinData::ReceiverJoinData (std::string const & caption,
									std::string const & memberName,
									std::string const & hubId)
	: _memberName (memberName),
	  _hubId (hubId),
	  _caption (caption),
	  _receiver (true),
	  _fullMember (false),
	  _trial (true)
{}

bool ReceiverJoinCtrl::OnInitDialog () throw (Win::Exception)
{
	_caption.Init (GetWindow (), IDC_JOIN_CAPTION);
	_fullMember.Init (GetWindow (), IDC_FULL_MEMBER);
	_receiver.Init (GetWindow (), IDC_RECEIVER);
	_licensed.Init (GetWindow (), IDC_LICENSED);
	_licensePool.Init (GetWindow (), IDC_POOL);

	_caption.SetText (_dlgData.GetCaption ());
	if (_dlgData.IsReceiver ())
	{
		_receiver.Check ();
		if (!_distributorPool.empty ())
		{
			_licensed.Check ();
		}
		else
		{
			_licensed.Disable ();
		}
		_licensePool.SetText (_distributorPool.GetLicensesLeftText ().c_str ());
	}
	else
	{
		_licensed.Disable ();
		if (_dlgData.IsFullMember ())
			_fullMember.Check ();
	}
	return true;
}

bool ReceiverJoinCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_RECEIVER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_licensed.Enable ();
			return true;
		}
		break;
	case IDC_FULL_MEMBER:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_licensed.Disable ();
			return true;
		}
		break;
	}
    return false;
}

bool ReceiverJoinCtrl::OnApply () throw ()
{
	if (_fullMember.IsChecked ())
	{
		std::string info ("Are you sure you want to accept ");
		info += _dlgData.GetName ();
		info += " (";
		info += _dlgData.GetHubId ();
		info += ") as a full member?\n\n";
		info += "Full members are able to see membership data of all other receivers"
				" in the project and make modifications to the project";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
												   Out::PromptStyle (Out::YesNo, Out::No, Out::Question),
												   GetWindow ());
		if (userChoice == Out::No)
			return false;
	}
	_dlgData.SetReceiver (_receiver.IsChecked ());
	_dlgData.SetFullMember (_fullMember.IsChecked ());
	_dlgData.SetTrial (!_licensed.IsChecked ());
	EndOk ();
	return true;
}
