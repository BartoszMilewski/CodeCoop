// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "precompiled.h"
#include "EmailOptionsPage.h"
#include "ConfigDlgData.h"
#include "EmailMan.h"
#include "OutputSink.h"
#include "Validators.h"

#include <StringOp.h>

bool EmailOptionsHandler::OnInitDialog () throw (Win::Exception)
{
	_maxEmailSize.Init    (GetWindow (), IDC_MAXEMAIL);
	_autoReceive.Init (GetWindow (), IDC_AUTO_RECEIVE);
	_autoReceivePeriod.Init (GetWindow (), IDC_AUTO_RECEIVE_PERIOD);
	_autoReceiveSpin.Init (GetWindow (), IDC_AUTO_RECEIVE_PERIOD_SPIN);

	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	_maxEmailSize.SetText (ToString (emailCfg.GetMaxEmailSize ()).c_str ());
	_autoReceiveSpin.SetRange (Email::MinAutoReceivePeriodInMin,
							   Email::MaxAutoReceivePeriodInMin);
	_autoReceivePeriod.LimitText (4);
	unsigned long autoReceivePeriod = emailCfg.GetAutoReceivePeriodInMin ();
	_autoReceiveSpin.SetPos (autoReceivePeriod);
	if (autoReceivePeriod != 0)
	{
		_autoReceive.Check ();
	}
	else
	{
		_autoReceivePeriod.Disable ();
		_autoReceiveSpin.Disable ();
	}
	return true;
}

bool EmailOptionsHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (IDC_AUTO_RECEIVE == ctrlId)
	{
		if (_autoReceive.IsChecked ())
		{
			_autoReceivePeriod.Enable ();
			_autoReceiveSpin.Enable ();
		}
		else
		{
			_autoReceivePeriod.Disable ();
			_autoReceiveSpin.Disable ();
		}
		return true;
	}
	return false;
}

void EmailOptionsHandler::RetrieveData (bool acceptPage)
{
	if (acceptPage)
	{
		Email::RegConfig & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
		unsigned userValue = 0;
		if (_autoReceive.IsChecked ())
			_autoReceivePeriod.GetUnsigned (userValue);

		emailCfg.SetAutoReceivePeriodInMin (userValue);

		userValue = 0;
		_maxEmailSize.GetUnsigned (userValue);
		emailCfg.SetMaxEmailSize (userValue);
	}
}

bool EmailOptionsHandler::Validate () const
{
	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	unsigned long autoReceivePeriod = emailCfg.GetAutoReceivePeriodInMin ();
	if (autoReceivePeriod != 0)
	{
		if (autoReceivePeriod < Email::MinAutoReceivePeriodInMin ||
			autoReceivePeriod > Email::MaxAutoReceivePeriodInMin)
		{
			std::string msg ("Please specify a valid automatic receive period\n(from ");
			msg += ToString (Email::MinAutoReceivePeriodInMin);
			msg += " to ";
			msg += ToString (Email::MaxAutoReceivePeriodInMin);
			msg += " minutes).";
			TheOutput.Display (msg.c_str ());
			return false;
		}
	}

	if (!ChunkSizeValidator (emailCfg.GetMaxEmailSize ()).IsInValidRange ())
	{
		std::string info ("Please specify a valid maximum size of script attachments\n(from ");
		info += ChunkSizeValidator::GetMinChunkSizeDisplayString ();
		info += " to ";
		info += ChunkSizeValidator::GetMaxChunkSizeDisplayString ();
		info += ").";
		TheOutput.Display (info.c_str ());
		return false;
	}

	return true;
}
