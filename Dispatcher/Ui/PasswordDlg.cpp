//------------------------------------
//  (c) Reliable Software, 2006 - 2008
//------------------------------------

#include "precompiled.h"
#include "PasswordDlg.h"
#include "resource.h"
#include "OutputSink.h"
#include "EmailConfig.h"
#include "EmailAccount.h"

EmailPasswordCtrl::EmailPasswordCtrl ()
	: Dialog::ControlHandler (IDD_EMAIL_PASSWORD),
	  _useSmtpPassword (false)
{}

bool EmailPasswordCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_emailAddress.Init (dlgWin, IDC_PASSWORD_EMAIL);
	_userName.Init (dlgWin, IDC_PASSWORD_USER);
	_pop3PasswordEdit.Init (dlgWin, IDC_PASSWORD_POP3);
	_pop3PasswordVerify.Init (dlgWin, IDC_PASSWORD_POP3_VERIFY);
	_smtpPasswordEdit.Init (dlgWin, IDC_PASSWORD_SMTP);
	_smtpPasswordVerify.Init (dlgWin, IDC_PASSWORD_SMTP_VERIFY);
	_smtpUsesPassword.Init (dlgWin, IDC_PASSWORD_SMTP_USE);
	_sameAsPop3.Init (dlgWin, IDC_PASSWORD_SAMEASPOP3);
	_different.Init (dlgWin, IDC_PASSWORD_DIFFERENT);

	Email::RegConfig emailCfg;
	Pop3Account pop3Account (emailCfg, std::string (), false); // don't read password
	_userName.SetText (pop3Account.GetUser ());

	if (emailCfg.IsSmtpRegKey ())
	{
		SmtpAccount smtpAccount (emailCfg, false); // don't read password
		_emailAddress.SetText (smtpAccount.GetSenderAddress ());
		_smtpUsesPassword.Check ();
		_sameAsPop3.Check ();
	}
	else
	{
		_emailAddress.SetText (pop3Account.GetUser ());
		_smtpUsesPassword.UnCheck ();
		_sameAsPop3.Disable ();
		_different.Disable ();
	}
	_smtpPasswordEdit.Disable ();
	_smtpPasswordVerify.Disable ();

	// Make sure that dialog is visible
	dlgWin.SetForeground ();
	return true;
}

bool EmailPasswordCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_PASSWORD_SMTP_USE)
	{
		if (_smtpUsesPassword.IsChecked ())
		{
			_sameAsPop3.Enable ();
			_different.Enable ();
			if (_different.IsChecked ())
			{
				_smtpPasswordEdit.Enable ();
				_smtpPasswordVerify.Enable ();
			}
		}
		else
		{
			_sameAsPop3.Disable ();
			_different.Disable ();
			_smtpPasswordEdit.Disable ();
			_smtpPasswordVerify.Disable ();
		}
	}
	else if (ctrlId == IDC_PASSWORD_SAMEASPOP3)
	{
		if (_sameAsPop3.IsChecked ())
		{
			_smtpPasswordEdit.Disable ();
			_smtpPasswordVerify.Disable ();
		}
		else
		{
			_smtpPasswordEdit.Enable ();
			_smtpPasswordVerify.Enable ();
		}
	}
	else if (ctrlId == IDC_PASSWORD_DIFFERENT)
	{
		if (_different.IsChecked ())
		{
			_smtpPasswordEdit.Enable ();
			_smtpPasswordVerify.Enable ();
		}
		else
		{
			_smtpPasswordEdit.Disable ();
			_smtpPasswordVerify.Disable ();
		}
	}

	return true;
}

bool EmailPasswordCtrl::OnApply () throw ()
{
	bool validPassword = true;
	_pop3Password = _pop3PasswordEdit.GetTrimmedString ();
	if (_pop3Password == _pop3PasswordVerify.GetTrimmedString ())
	{
		if (_smtpUsesPassword.IsChecked ())
		{
			_useSmtpPassword = true;
			if (_sameAsPop3.IsChecked ())
			{
				_smtpPassword = _pop3Password;
			}
			else
			{
				_smtpPassword = _smtpPasswordEdit.GetTrimmedString ();
				if (_smtpPassword != _smtpPasswordVerify.GetTrimmedString ())
				{
					TheOutput.Display ("SMTP login password verification failed. Please, re-type SMTP passwords.");
					validPassword = false;
				}
			}
		}
		else
		{
			_useSmtpPassword = false;
		}
	}
	else
	{
		TheOutput.Display ("POP3 login password verification failed. Please, re-type POP3 passwords.");
		validPassword = false;
	}
	if (validPassword)
		EndOk ();
	return true;
}
