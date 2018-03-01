// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "precompiled.h"
#include "EmailPropertiesSheet.h"
#include "EmailConfigData.h"
#include "ScriptProcessorConfig.h"
#include "EmailDiag.h"
#include "EmailRegistry.h"
#include "EmailMan.h"
#include "EmailAccount.h"
#include "OutputSink.h"
#include "Validators.h"
#include "Prompter.h"

EmailPropertiesSheet::EmailPropertiesSheet (
	Win::Dow::Handle win,
	Email::Manager & emailMan,
	std::string const & myEmail,
	Win::MessagePrepro * msgPrepro)
: PropPage::Sheet  (win, "E-mail Options"),
	_generalCtrl (emailMan),
	_diagnosticsCtrl (myEmail, emailMan, msgPrepro),
	_processingCtrl (emailMan)
{
	SetNoApplyButton ();
	SetNoContextHelp ();

	AddPage (_generalCtrl, "General");
	AddPage (_diagnosticsCtrl, "Diagnostics");
	AddPage (_processingCtrl, "External Processing");
}

// ======================
bool EmailDiagnosticsCtrl::OnInitDialog () throw (Win::Exception)
{
	_status.Init (GetWindow (), IDC_TEST_STATUS);
	_progressBar.Init (GetWindow (), IDC_TEST_PROGRESS);
	_start.Init (GetWindow (), IDC_TEST_START);
	_stop.Init (GetWindow (), IDC_TEST_STOP);

	_diagProgress.Init (&_progressBar, _msgPrepro);

	_status.SetReadonly (true);
	_stop.Disable ();
	_start.SetFocus ();

	return true;
}

bool EmailDiagnosticsCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	try
	{
		switch (ctrlId)
		{
		case IDC_TEST_START:
			RunDiagnostics ();
			return true;
		case IDC_TEST_STOP:
			_diagProgress.Cancel ();
			return true;
		}
		return false;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("Unknown exception.");
	}
	return true;
}

void EmailDiagnosticsCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	_status.SetReadonly (false);
	_status.Clear ();
	_status.SetReadonly (true);
}

void EmailDiagnosticsCtrl::RunDiagnostics ()
{
	// Revisit: block switching to other page, Ok'ing sheet, 
	//			enable Stop button, disable Start button
	DiagFeedback feedback (_status);
	feedback.Clear ();

	Email::Diagnostics emailDiag (_myEmail, feedback, _diagProgress);
	Email::Status status = emailDiag.Run (_emailMan);
	if (status != Email::NotTested)
		_emailMan.SetEmailStatus (status);
}

// ==================
bool EmailGeneralCtrl::OnInitDialog () throw (Win::Exception)
{
	_staticSendUsingMapi.Init (GetWindow (), IDC_STATIC_SEND_USING_MAPI);
	_staticNoSmtp.Init (GetWindow (), IDC_STATIC_NO_SMTP);
	_sendUsingMapi.Init (GetWindow (), IDC_RADIO_SEND_USING_MAPI);
	_sendUsingSmtp.Init (GetWindow (), IDC_RADIO_USING_SMTP);
	_smtpAccounts.Init (GetWindow (), IDC_SMTP);
	_maxEmailSize.Init (GetWindow (), IDC_MAXEMAIL);

	_staticRecvUsingMapi.Init (GetWindow (), IDC_STATIC_RECV_USING_MAPI);
	_staticNoPop3.Init (GetWindow (), IDC_STATIC_NO_POP3);
	_recvUsingMapi.Init (GetWindow (), IDC_RADIO_RECV_USING_MAPI);
	_recvUsingPop3.Init (GetWindow (), IDC_RADIO_USING_POP3);
	_pop3Accounts.Init (GetWindow (), IDC_POP3);
	_autoReceive.Init (GetWindow (), IDC_AUTO_RECEIVE);
	_autoReceivePeriod.Init (GetWindow (), IDC_AUTO_RECEIVE_PERIOD);
	_autoReceiveSpin.Init (GetWindow (), IDC_AUTO_RECEIVE_PERIOD_SPIN);

	ResetControls ();

	return true;
}

void EmailGeneralCtrl::ResetControls ()
{
	if (_emailCfg.IsUsingSmtp () || _emailCfg.IsSmtpRegKey ())
	{
		_staticSendUsingMapi.Hide ();
		_staticNoSmtp.Hide ();
		_sendUsingMapi.Show ();
		_sendUsingSmtp.Show ();
		_smtpAccounts.SetText ("SMTP Settings");
		if (_emailCfg.IsUsingSmtp ())
		{
			_sendUsingSmtp.Check ();
		}
		else
		{
			_sendUsingMapi.Check ();
			_smtpAccounts.Disable ();
		}
	}
	else
	{
		_sendUsingMapi.Hide ();
		_sendUsingSmtp.Hide ();
		_smtpAccounts.SetText ("Start using SMTP");
	}

	if (_emailCfg.IsUsingPop3 () || _emailCfg.IsPop3RegKey ())
	{
		_staticRecvUsingMapi.Hide ();
		_staticNoPop3.Hide ();
		_recvUsingMapi.Show ();
		_recvUsingPop3.Show ();
		_pop3Accounts.SetText ("POP3 Settings");
		if (_emailCfg.IsUsingPop3 ())
		{
			_recvUsingPop3.Check ();
		}
		else
		{
			_recvUsingMapi.Check ();
			_pop3Accounts.Disable ();
		}
	}
	else
	{
		_recvUsingMapi.Hide ();
		_recvUsingPop3.Hide ();
		_pop3Accounts.SetText ("Start using POP3");
	}

	// revisit: implement SetUnsigned
	_maxEmailSize.SetText (ToString (_emailCfg.GetMaxEmailSize ()).c_str ());
	_autoReceiveSpin.SetRange (Email::MinAutoReceivePeriodInMin,
							   Email::MaxAutoReceivePeriodInMin);
	_autoReceivePeriod.LimitText (4);
	_autoReceiveSpin.SetPos (_emailCfg.GetAutoReceivePeriodInMin ());
	if (_emailCfg.IsAutoReceive ())
	{
		_autoReceive.Check ();
	}
	else
	{
		_autoReceive.UnCheck ();
		_autoReceivePeriod.Disable ();
		_autoReceiveSpin.Disable ();
	}
}

void EmailGeneralCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	Registry::DefaultEmailClient emailClient;
	std::string emailClientName = emailClient.GetName ();
	std::string clientTxt ("Currently the default e-mail program is ");
	if (emailClientName.empty ())
		clientTxt += "not specified";
	else
		clientTxt += emailClientName;
	clientTxt += '.';
	if (!_emailCfg.IsUsingSmtp ())
	{
		_staticSendUsingMapi.SetText (clientTxt);
	}
	if (!_emailCfg.IsUsingPop3 ())
	{
		_staticRecvUsingMapi.SetText (clientTxt);
	}
}

void EmailGeneralCtrl::OnKillActive (long & result) throw (Win::Exception)
{
	result = 0;
	RetrieveData ();
	if (Validate ())
		_emailMan.Refresh ();
	else
		result = -1;
}

bool EmailGeneralCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	try
	{
		switch (ctrlId)
		{
		case IDC_RADIO_USING_SMTP:
			_smtpAccounts.Enable ();
			break;
		case IDC_RADIO_SEND_USING_MAPI:
			_smtpAccounts.Disable ();
			break;
		case IDC_RADIO_USING_POP3:
			_pop3Accounts.Enable ();
			break;
		case IDC_RADIO_RECV_USING_MAPI:
			_pop3Accounts.Disable ();
			break;
		case IDC_AUTO_RECEIVE:
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
		case IDC_SMTP:
			{
				SmtpAccount account (_emailCfg);
				SmtpCtrl ctrl (account);
				if (ThePrompter.GetData (ctrl))
				{
					account.Save (_emailCfg);
					_emailCfg.SetIsUsingSmtp (true);
					_emailMan.Refresh ();
					ResetControls ();
				}
			}
			return true;
		case IDC_POP3:
			{
				Pop3Account account (_emailCfg, std::string ());// Default POP3 account
				Pop3Ctrl ctrl (account);
				if (ThePrompter.GetData (ctrl))
				{
					account.Save (_emailCfg);
					_emailCfg.SetIsUsingPop3 (true);
					_emailMan.Refresh ();
					ResetControls ();
				}
			}
			return true;
		}
		return false;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("Unknown exception.");
	}
	return true;
}

void EmailGeneralCtrl::RetrieveData ()
{
	_emailCfg.SetIsUsingSmtp (_sendUsingSmtp.IsVisible () && _sendUsingSmtp.IsChecked ());
	_emailCfg.SetIsUsingPop3 (_recvUsingPop3.IsVisible () && _recvUsingPop3.IsChecked ());

	if (_autoReceive.IsChecked ())
	{
		unsigned period = 0;
		_autoReceivePeriod.GetUnsigned (period);
		_emailCfg.SetAutoReceivePeriodInMin (period);
	}
	else
	{
		_emailCfg.SetAutoReceive (false);
		_emailCfg.SetAutoReceivePeriodInMin (0);
	}

	unsigned max = 0;
	_maxEmailSize.GetUnsigned (max);
	_emailCfg.SetMaxEmailSize (max);
}

bool EmailGeneralCtrl::Validate () const
{
	if (_emailCfg.IsUsingSmtp ())
	{
		SmtpAccount account (_emailCfg);
		if (!account.IsValid ())
		{
			TheOutput.Display ("Your SMTP account has incorrect settings.\n"
							   "Configure the account under \"SMTP Settings\".", 
							   Out::Information, 
							   GetWindow ());
			return false;
		}
	}
	if (_emailCfg.IsUsingPop3 ())
	{
		Pop3Account account (_emailCfg, std::string ());
		if (!account.IsValid ())
		{
			TheOutput.Display ("Your POP3 account has incorrect settings.\n"
				"Configure the account under \"POP3 Settings\".", 
				Out::Information, 
				GetWindow ());
			return false;
		}
	}

	EmailConfigData emailData (_emailCfg);
	return emailData.Validate ();
}

// ========================
void EmailProcessingCtrl::OnApply (long & result)
{
	result = 0;
	RetrieveData ();
	if (!CheckDataAndDisplayErrors ())
		result = -1;
}

bool EmailProcessingCtrl::OnInitDialog () throw (Win::Exception)
{
	_preproProgram.Init (GetWindow (), IDC_PREPRO_PROGRAM);
	_preproResult.Init (GetWindow (), IDC_PREPRO_RESULT);
	_postproProgram.Init (GetWindow (), IDC_POSTPRO_PROGRAM);
	_postproExt.Init (GetWindow (), IDC_POSTPRO_EXT);
	_preproNeedsProjName.Init (GetWindow (), IDC_CHECK);
	_preproSendUnprocessed.Init (GetWindow (), IDC_SEND_UNPROCESSED);

	ScriptProcessorConfig cfg;
	_emailMan.GetEmailConfig ().ReadScriptProcessorConfig (cfg);
	_preproProgram.SetString  (cfg.GetPreproCommand  ());
	_preproResult.SetString   (cfg.GetPreproResult   ());
	_postproProgram.SetString (cfg.GetPostproCommand ());
	_postproExt.SetString     (cfg.GetPostproExt     ());
	if (cfg.PreproNeedsProjName ())
		_preproNeedsProjName.Check ();

	if (cfg.CanSendUnprocessed ())
		_preproSendUnprocessed.Check ();

	return true;
}

void EmailProcessingCtrl::RetrieveData ()
{
	ScriptProcessorConfig cfg;
	cfg.SetPreproCommand (_preproProgram.GetString ());
	cfg.SetPreproResult (_preproResult.GetString ());
	cfg.SetPostproCommand (_postproProgram.GetString ());
	cfg.SetPostproExt (_postproExt.GetString ());
	cfg.SetPreproNeedsProjName (_preproNeedsProjName.IsChecked ());
	cfg.SetCanSendUnprocessed (_preproSendUnprocessed.IsChecked ());
	_emailMan.GetEmailConfig ().SaveScriptProcessorConfig (cfg);
}

bool EmailProcessingCtrl::CheckDataAndDisplayErrors () const
{
	ScriptProcessorConfig cfg;
	_emailMan.GetEmailConfig ().ReadScriptProcessorConfig (cfg);
	ScriptProcessorValidator validator (cfg);
	if (validator.IsValid ())
		return true;

	validator.DisplayErrors ();
	return false;
}

// sub-dialogs
bool SmtpCtrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_server.Init (GetWindow (), IDC_SERVER);
	_server.SetText (_dlgData.GetServer ().c_str ());
	_port.Init (GetWindow (), IDC_PORT_NUMBER);
	_port.LimitText (6);
	_port.SetText (ToString (_dlgData.GetPort ()));
	_useSSL.Init (GetWindow (), IDC_USE_SSL);
	if (_dlgData.UseSSL ())
		_useSSL.Check ();
	_email.Init (GetWindow (), IDC_EMAIL);
	_email.SetText (_dlgData.GetSenderAddress ().c_str ());
	_user.Init (GetWindow (), IDC_USER);
	_user.SetText (_dlgData.GetUser ().c_str ());
	_password.Init (GetWindow (), IDC_PASSW);
	_password.SetText (_dlgData.GetPassword ().c_str ());
	_password2.Init (GetWindow (), IDC_PASSW2);
	_password2.SetText (_dlgData.GetPassword ().c_str ());
	_isAuthenticate.Init (GetWindow (), IDC_AUTH);
	if (_dlgData.IsAuthenticate ())
	{
		_isAuthenticate.Check ();
	}
	else
	{
		_user.SetReadonly (true);
		_password.SetReadonly (true);
		_password2.SetReadonly (true);
	}
	return true;
}

bool SmtpCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_AUTH:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_isAuthenticate.IsChecked ())
			{
				_user.SetReadonly (false);
				_password.SetReadonly (false);
				_password2.SetReadonly (false);
			}
			else
			{
				_user.SetReadonly (true);
				_password.SetReadonly (true);
				_password2.SetReadonly (true);
			}
		}
		break;
	case IDC_USE_SSL:
		if (_useSSL.IsChecked () && !_isSSLSupported)
		{
			TheOutput.Display ("SSL secured connections are not supported on this operating system.");
			_useSSL.UnCheck ();
		}
        // Fix: Don't overwrite user data
		//_port.SetText (ToString (_dlgData.GetDefaultPort (_useSSL.IsChecked ())));
		break;
	};

	return false;
}

bool SmtpCtrl::OnApply () throw ()
{
	SmtpAccount newSmtpAccount (_server.GetTrimmedString (),
								_user.GetTrimmedString (),
								_password.GetTrimmedString ());
	unsigned int port = 0;
	_port.GetUnsigned (port);
	newSmtpAccount.SetPort (port);
	newSmtpAccount.SetUseSSL (_useSSL.IsChecked ());
	newSmtpAccount.SetAuthenticate (_isAuthenticate.IsChecked ());
	newSmtpAccount.SetSenderAddress (_email.GetTrimmedString ());

	std::string errMsg = newSmtpAccount.Validate (_password2.GetTrimmedString ());
	if (errMsg.empty ())
	{
		if (!_dlgData.IsEqual (newSmtpAccount))
			_dlgData.Reset (newSmtpAccount);
		EndOk ();
	}
	else
	{
		TheOutput.Display (errMsg.c_str (), Out::Error);
	}
	return true;
}

bool Pop3Ctrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_server.Init (GetWindow (), IDC_SERVER);
	_server.SetText (_dlgData.GetServer ().c_str ());
	_port.Init (GetWindow (), IDC_PORT_NUMBER);
	_port.SetText (ToString (_dlgData.GetPort ()));
	_port.LimitText (6);
	_useSSL.Init (GetWindow (), IDC_USE_SSL);
	if (_dlgData.UseSSL ())
		_useSSL.Check ();
	_user.Init (GetWindow (), IDC_USER);
	_user.SetText (_dlgData.GetUser ().c_str ());
	_password.Init (GetWindow (), IDC_PASSW);
	_password.SetText (_dlgData.GetPassword ().c_str ());
	_password2.Init (GetWindow (), IDC_PASSW2);
	_password2.SetText (_dlgData.GetPassword ().c_str ());
	_doDelete.Init (GetWindow (), IDC_DELETE);
	_dontDelete.Init (GetWindow (), IDC_DONT_DELETE);
	if (_dlgData.IsDeleteNonCoopMsg ())
		_doDelete.Check ();
	else
		_dontDelete.Check ();
	
	_help.Init (GetWindow (), IDC_HELP_BUTTON);

	return true;
}

bool Pop3Ctrl::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_HELP_BUTTON:
		OpenHelp ();
		break;
	case IDC_USE_SSL:
		if (_useSSL.IsChecked () && !_isSSLSupported)
		{
			TheOutput.Display ("SSL secured connections are not supported on this operating system.");
			_useSSL.UnCheck ();
		}
        // Fixed: Don't overwrite user's setting!
		//_port.SetText (ToString (_dlgData.GetDefaultPort (_useSSL.IsChecked ())));
	}
	return true;
}

bool Pop3Ctrl::OnApply () throw ()
{
	Pop3Account newPop3Account (_server.GetTrimmedString (),
								_user.GetTrimmedString (),
								_password.GetTrimmedString (),
								_useSSL.IsChecked (),
								std::string ());
	unsigned int port = 0;
	_port.GetUnsigned (port);
	newPop3Account.SetPort (port);
	newPop3Account.SetDeleteNonCoopMsg (_doDelete.IsChecked ());

	std::string errMsg = newPop3Account.Validate (_password2.GetTrimmedString ());
	if (errMsg.empty ())
	{
		if (!_dlgData.IsEqual (newPop3Account))
			_dlgData.Reset (newPop3Account);
		EndOk ();
	}
	else
	{
		TheOutput.Display (errMsg.c_str (), Out::Error);
	}
	return true;
}
