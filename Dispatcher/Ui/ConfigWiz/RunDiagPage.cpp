// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "precompiled.h"
#include "RunDiagPage.h"
#include "ConfigDlgData.h"
#include "DiagFeedback.h"
#include "EmailMan.h"
#include "EmailDiag.h"
#include "EmailConfig.h"
#include "OutputSink.h"
#include "DefaultEmailAddress.h"
#include "EmailPromptCtrl.h"
#include "Prompter.h"

#include <Mail/EmailAddress.h>

bool RunDiagHandler::OnInitDialog () throw (Win::Exception)
{
	Assert (_wizardData.GetNewTopology ().IsHubOrPeer ());

	_dontAsk.Init (GetWindow (), IDC_CHECK);
	_status.Init (GetWindow (), IDC_FEEDBACK);
	_emailAddr.Init (GetWindow (), IDC_TEST_MY_EMAIL);
	_diag.Init (GetWindow (), IDC_DIAG);

	// Topology changes if diagnostics fails
	_modified.Set (ConfigData::bitTopologyCfg);
	// These two are changed in a case the original hub id is empty or "Unknown"
	_modified.Set (ConfigData::bitHubId);
	_modified.Set (ConfigData::bitInterClusterTransportToMe);

	return true;
}

void RunDiagHandler::OnSetActive (long & result)
{
	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	if (emailCfg.IsUsingPop3 ())
	{
		// We came here after configuring SMTP/POP3
		_emailAddr.SetString (_wizardData.GetNewConfig ().GetHubId ());
		_emailAddr.SetReadonly (true);
	}
	else
	{
		// Using MAPI
		if (Email::IsValidAddress (_previousHubId))
			_emailAddr.SetString (_previousHubId);
		else
			_emailAddr.SetString (Email::GetDefaultAddress (_wizardData.GetEmailMan ()));
	}
	DiagFeedback feedback (_status);
	feedback.Clear ();
	result = 0;
	BaseWizardHandler::OnSetActive (result);
}

bool RunDiagHandler::ValidateEmailAddress (std::string & emailAddress) const
{
	if (emailAddress.empty ())
	{
		// Ask the user
		EmailPromptCtrl ctrl (emailAddress);
		if (!ThePrompter.GetData (ctrl))
			return false;
	}
	if (!Email::IsValidAddress (emailAddress))
		return false;

	return true;
}

bool RunDiagHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_DIAG)
	{
		DiagFeedback feedback (_status);
		feedback.Clear ();
		std::string emailAddress = _emailAddr.GetTrimmedString ();
		if (ValidateEmailAddress (emailAddress))
		{
			_emailAddr.SetString (emailAddress);
			_diag.Disable ();
			NullDiagProgress progress;
			Email::Diagnostics emailDiag (emailAddress, feedback, progress);
			Email::Manager  & emailMan = _wizardData.GetEmailMan ();
			Email::Status status = emailDiag.Run (emailMan);
			emailMan.SetEmailStatus (status);
			if (status != Email::NotTested)
				_isDiagPerformed = true;
			_diag.Enable ();
		}
		else
		{
			std::string info ("The following e-mail address is invalid: ");
			info += emailAddress;
			feedback.Display (info.c_str ());
		}
		return true;
	}
	return false;
}

void RunDiagHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	ConfigData & config = _wizardData.GetNewConfig ();
	Topology const & topology = config.GetTopology ();

	if (!acceptPage)
		return;

	Email::Manager & emailMan = _wizardData.GetEmailMan ();
	_isDontAsk = false;
	if (_dontAsk.IsChecked ())
	{
		// user assures us that his e-mail program works with Dispatcher
		emailMan.SetEmailStatus (Email::Succeeded);
		_isDontAsk = true;
	}

	if (_isDiagPerformed)
	{
		if (emailMan.GetEmailStatus () == Email::Failed)
			return;
	}
	else
	{
		if (!_isDontAsk)
			return;
	}

	Email::RegConfig const & emailCfg = emailMan.GetEmailConfig ();
	if (!emailCfg.IsUsingPop3 ())
	{
		std::string userEmailFromDlg = _emailAddr.GetTrimmedString ();
		Transport newTransport (userEmailFromDlg);
		if (newTransport.IsEmail ())
		{
			config.SetHubId (userEmailFromDlg);
			config.SetInterClusterTransportToMe (newTransport);
		}
		else
		{
			config.SetHubId (std::string ());
		}
	}
}

bool RunDiagHandler::Validate () const
{
	ConfigData & config = _wizardData.GetNewConfig ();

	if (_isDiagPerformed)
	{
		if (_wizardData.GetEmailMan ().GetEmailStatus () == Email::Failed)
		{
			TheOutput.Display ("You cannot continue when the diagnostics failed.");
			return false;
		}
	}
	else if (!_isDontAsk)
	{
		TheOutput.Display ("To complete e-mail configuration you have to run diagnostics.");
		return false;
	}

	Transport newTransport (config.GetHubId ());
	if (!newTransport.IsEmail ())
	{
		TheOutput.Display ("You have to provide a valid e-mail address.");
		return false;
	}

	if (_previousHubId.empty () || IsNocaseEqual (_previousHubId, "Unknown"))
		return true;

	if (!IsNocaseEqual (_previousHubId, config.GetHubId ()) && !config.GetTopology ().IsPeer ())
	{
		// user changed hub id, confirm his action
		std::string msg = "Your previous e-mail address was:\n\n";
		msg += _previousHubId;
		msg += "\n\nAre you sure you want to change it to:\n\n";
		msg += config.GetHubId ();
		msg += " ?\n\nIf so, you'll have to visit all projects and\n"
			   "accept the hub ID change in order to\n"
			   "notify other project members.";
		
		if (TheOutput.Prompt (msg.c_str (), Out::PromptStyle (Out::YesNo)) == Out::No)
			return false;
	}
	return true;
}
