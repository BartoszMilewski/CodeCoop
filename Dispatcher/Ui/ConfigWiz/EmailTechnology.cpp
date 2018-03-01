// ----------------------------------
// (c) Reliable Software, 2006 - 2009
// ----------------------------------

#include "precompiled.h"
#include "EmailTechnology.h"
#include "Gmail.h"
#include "EmailAccount.h"
#include "EmailRegistry.h"
#include "EmailOptionsSheet.h"
#include "EmailMan.h"
#include "OutputSink.h"

#include <Mail/EmailAddress.h>
#include <Net/NetShare.h>

bool EmailSelectionHandler::OnInitDialog () throw (Win::Exception)
{
	_emailProgram.Init (GetWindow (), IDC_EMAIL_PROGRAM);
	_pop3Smtp.Init (GetWindow (), IDC_EMAIL_POP3SMTP);
	_gmail.Init (GetWindow (), IDC_EMAIL_GMAIL);

	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	if (emailCfg.IsUsingPop3 ())
	{
		Pop3Account pop3Account (emailCfg, std::string ());
		if (IsNocaseEqual (pop3Account.GetServer ().c_str (), Email::GmailPop3Server))
			_gmail.Check ();
		else
			_pop3Smtp.Check ();
	}
	else
	{
		Registry::DefaultEmailClient defaultEmailClient;
		if (defaultEmailClient.IsMapiEmailClient ())
			_emailProgram.Check ();
		else
			_pop3Smtp.Check ();
	}

	return true;
}

void EmailSelectionHandler::OnSetActive (long & result) throw (Win::Exception)
{
	BaseWizardHandler::OnSetActive (result);
	if (_navigator->WasLastMoveFwd ())
		_wizardData.GetEmailMan ().BeginEdit ();
}

bool EmailSelectionHandler::Validate () const
{
	if (_emailProgram.IsChecked ())
	{
		// Check if we have MAPI enabled default e-mail program
		Registry::DefaultEmailClient defaultEmailClient;
		if (!defaultEmailClient.IsMapiEmailClient ())
		{
			std::string msg ("Your default e-mail program is '");
			msg += defaultEmailClient.GetName ();
			msg += "'.\nCode Co-op cannot determine if this program correctly supports MAPI or Simple MAPI interface.\n"
				   "It will probably not work with Code Co-op.\n\n"
				   "Do you want to continue anyway?";
			Out::Answer userChoice = TheOutput.Prompt (msg.c_str (),
													   Out::PromptStyle (Out::YesNo,
																		 Out::No,
																		 Out::Question),
													   GetWindow ());
			if (userChoice == Out::No)
				return false;	// Don't go to the next page
		}
	}
	return true;
}

void EmailSelectionHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	if (!acceptPage)
	{
		// User is backing up from email configuration
		_wizardData.GetEmailMan ().AbortEdit ();
		return;
	}

	bool const isPop3Smtp = !_emailProgram.IsChecked ();
	Email::RegConfig & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	emailCfg.SetIsUsingPop3 (isPop3Smtp);
	emailCfg.SetIsUsingSmtp (isPop3Smtp);
	_wizardData.GetEmailMan ().Refresh ();
}

long  EmailSelectionHandler::ChooseNextPage () const throw (Win::Exception)
{
	if (_emailProgram.IsChecked ())
		return IDD_WIZARD_DIAGNOSTICS;
	else if (_pop3Smtp.IsChecked ())
		return IDD_WIZARD_EMAIL_POP3SMTP;
	else
		return IDD_WIZARD_EMAIL_GMAIL;
}

bool HubEmailSelectionHandler::OnInitDialog () throw (Win::Exception)
{
	EmailSelectionHandler::OnInitDialog ();

	_hubShare.Init (GetWindow (), IDC_HUB_SHARE);
    
	ConfigData & newConfig = _wizardData.GetNewConfig ();
	std::string myPublicInboxShareName = newConfig.GetActiveIntraClusterTransportToMe ().GetRoute ();
	if (myPublicInboxShareName.empty ())
	{
		Net::LocalPath sharePath;
		sharePath.DirDown ("CODECOOP");
		myPublicInboxShareName = sharePath.ToString ();
		Transport transportToMe (myPublicInboxShareName);
		newConfig.SetActiveIntraClusterTransportToMe (transportToMe);
		_modified.Set (ConfigData::bitInterClusterTransportToMe);
	}
	_hubShare.SetText (myPublicInboxShareName.c_str ());

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_SHARE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

EmailPop3SmtpHandler::EmailPop3SmtpHandler (ConfigDlgData & config, unsigned pageId)
	: BaseWizardHandler (config, pageId),
	 _isValidData (false)
{
}

bool EmailPop3SmtpHandler::OnInitDialog () throw (Win::Exception)
{
	_pop3User.Init (GetWindow (), IDC_POP3_USER);
	_pop3Password.Init (GetWindow (), IDC_POP3_PASSW);
	_pop3PasswordVerify.Init (GetWindow (), IDC_POP3_PASSWORD_VERIFY);
	_pop3Server.Init (GetWindow (), IDC_POP3SERVER);

	_emailAddress.Init (GetWindow (), IDC_EMAIL);
	_smtpUser.Init (GetWindow (), IDC_SMTP_USER);
	_smtpPassword.Init (GetWindow (), IDC_SMTP_PASSW);
	_smtpPasswordVerify.Init (GetWindow (), IDC_SMTP_PASSWORD_VERIFY);
	_smtpServer.Init (GetWindow (), IDC_SMTPSERVER);
	_isAuthenticate.Init (GetWindow (), IDC_AUTH);
	_isSameAsPop3.Init (GetWindow (), IDC_AUTH_SAME_AS_POP3);
	_advancedPop3Smpt.Init (GetWindow (), IDC_ADVANCED_POP3SMTP);

	LoadCtrls ();
	ResetAuthenticateCtrls ();

	return true;
}

bool EmailPop3SmtpHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_AUTH:
		ResetAuthenticateCtrls ();
		return true;

	case IDC_AUTH_SAME_AS_POP3:
		if (_isSameAsPop3.IsChecked ())
		{
			_smtpUser.SetText (_pop3User.GetTrimmedString ());
			_smtpPassword.SetText (_pop3Password.GetTrimmedString ());
		}
		ResetAuthenticateCtrls ();
		return true;

	case IDC_ADVANCED_POP3SMTP:
		RetrieveData (true);
		if (Email::RunOptionsSheet (_emailAddress.GetTrimmedString (),
									_wizardData.GetEmailMan (),
									GetWindow (),
									_wizardData.GetMsgPrepro ()))
		{
			LoadCtrls ();
			ResetAuthenticateCtrls ();
		}
		return true;
	};
	return false;
}

void EmailPop3SmtpHandler::ResetAuthenticateCtrls ()
{
	bool const isAuth = _isAuthenticate.IsChecked ();

	_isSameAsPop3.Enable (isAuth);
	bool canEditSmtpAuth = isAuth && !_isSameAsPop3.IsChecked ();
	_smtpUser.Enable (canEditSmtpAuth);
	_smtpPassword.Enable (canEditSmtpAuth);
	_smtpPasswordVerify.Enable (canEditSmtpAuth);
}

void EmailPop3SmtpHandler::LoadCtrls ()
{
	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	Pop3Account pop3Account (emailCfg, std::string ());
	SmtpAccount smtpAccount (emailCfg);

	_pop3User.SetText (pop3Account.GetUser ());
	_pop3Password.SetText (pop3Account.GetPassword ());
	_pop3PasswordVerify.SetText (pop3Account.GetPassword ());
	_pop3Server.SetText (pop3Account.GetServer ());

	_emailAddress.SetText (smtpAccount.GetSenderAddress ());
	_smtpUser.SetText (smtpAccount.GetUser ());
	_smtpPassword.SetText (smtpAccount.GetPassword ());
	_smtpPasswordVerify.SetText (smtpAccount.GetPassword ());
	_smtpServer.SetText (smtpAccount.GetServer ());

	if (smtpAccount.IsAuthenticate ())
		_isAuthenticate.Check ();
	else
		_isAuthenticate.UnCheck ();

	if (pop3Account.GetUser () == smtpAccount.GetUser () &&
		pop3Account.GetPassword () == smtpAccount.GetPassword ())
	{
		_isSameAsPop3.Check ();
	}
}

bool EmailPop3SmtpHandler::Validate () const
{
	return _isValidData;
}

// We also validate and store in registry if valid
void EmailPop3SmtpHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	_isValidData = true;
	if (!acceptPage)
		return;

	bool changesDetected = false;
	Email::Manager & emailMan = _wizardData.GetEmailMan ();
	Email::RegConfig & emailCfg = emailMan.GetEmailConfig ();

	Pop3Account currentPop3Account (emailCfg, std::string ());
	Pop3Account newPop3Account (_pop3Server.GetTrimmedString (),
								_pop3User.GetTrimmedString (),
								_pop3Password.GetTrimmedString (),
								false,	// Not using SSL
								std::string ());	// Default account

	newPop3Account.SetPort (currentPop3Account.GetPort ());
	std::string errMsg = newPop3Account.Validate (_pop3PasswordVerify.GetTrimmedString ());
	if (errMsg.empty ())
	{
		// POP3 dialog settings are valid - are there any changes?
		if (!currentPop3Account.IsEqual (newPop3Account))
		{
			newPop3Account.Save (emailCfg);
			changesDetected = true;
		}
	}
	else
	{
		_isValidData = false;
		TheOutput.Display (errMsg.c_str ());
		return;
	}

	SmtpAccount currentSmtpAccount (emailCfg);

	std::string newSmtpUser;
	std::string newSmtpPassword;
	std::string newSmtpPasswordVerify;
	if (_isAuthenticate.IsChecked ())
	{
		if (_isSameAsPop3.IsChecked ())
		{
			newSmtpUser = newPop3Account.GetUser ();
			newSmtpPassword = newPop3Account.GetPassword ();
			newSmtpPasswordVerify = _pop3PasswordVerify.GetTrimmedString ();
		}
		else
		{
			newSmtpUser = _smtpUser.GetTrimmedString ();
			newSmtpPassword = _smtpPassword.GetTrimmedString ();
			newSmtpPasswordVerify = _smtpPasswordVerify.GetTrimmedString ();
		}
	}

	SmtpAccount newSmtpAccount (_smtpServer.GetTrimmedString (),
								newSmtpUser,
								newSmtpPassword);
	newSmtpAccount.SetPort (currentSmtpAccount.GetPort ());
	newSmtpAccount.SetUseSSL(currentSmtpAccount.UseSSL());
	newSmtpAccount.SetSenderAddress (_emailAddress.GetTrimmedString ());
	newSmtpAccount.SetAuthenticate (_isAuthenticate.IsChecked ());

	errMsg = newSmtpAccount.Validate (newSmtpPasswordVerify);
	if (errMsg.empty ())
	{
		// SMTP dialog settings are valid - are there any changes?
		if (!currentSmtpAccount.IsEqual (newSmtpAccount))
		{
			newSmtpAccount.Save (emailCfg);
			changesDetected = true;
		}
		if (currentSmtpAccount.GetSenderAddress () != newSmtpAccount.GetSenderAddress ())
		{
			ConfigData & newCfg = _wizardData.GetNewConfig ();
			newCfg.SetHubId (newSmtpAccount.GetSenderAddress ());
			newCfg.SetInterClusterTransportToMe (Transport (newSmtpAccount.GetSenderAddress (), Transport::Email));
		}
	}
	else
	{
		_isValidData = false;
		TheOutput.Display (errMsg.c_str ());
	}

	if (changesDetected)
		emailMan.Refresh ();
}

EmailGmailHandler::EmailGmailHandler (ConfigDlgData & config, unsigned pageId)
	: BaseWizardHandler (config, pageId),
	  _isValidData (false)
{
}

bool EmailGmailHandler::OnInitDialog () throw (Win::Exception)
{
	_user.Init (GetWindow (), IDC_USER);
	_password.Init (GetWindow (), IDC_PASSW);
	_passwordVerify.Init (GetWindow (), IDC_PASSWORD_VERIFY);

	Email::RegConfig const & emailCfg = _wizardData.GetEmailMan ().GetEmailConfig ();
	Pop3Account pop3Account (emailCfg, std::string ());
	std::string const emailAddress (pop3Account.GetUser ());
	std::string::size_type pos = emailAddress.find ('@');
	if ((pos != std::string::npos) && IsNocaseEqual (emailAddress.substr (pos + 1), Email::GmailDomain))
	{
		std::string username = emailAddress.substr (0, pos);
		_user.SetText (username);
		_password.SetText (pop3Account.GetPassword ());
		_passwordVerify.SetText (pop3Account.GetPassword ());
	}
	return true;
}

bool EmailGmailHandler::Validate () const
{
	return _isValidData;
}

// We also validate and store in registry if valid
void EmailGmailHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	_isValidData = true;
	if (!acceptPage)
		return;

	bool changesDetected = false;
	Email::Manager & emailMan = _wizardData.GetEmailMan ();
	Email::RegConfig & emailCfg = emailMan.GetEmailConfig ();

	Pop3Account currentPop3Account (emailCfg, std::string ());

	std::string user (_user.GetTrimmedString ());
	user += '@';
	user += Email::GmailDomain;
	Pop3Account newPop3Account (Email::GmailPop3Server,
								user,
								_password.GetTrimmedString (),
								true,	// Use SSL
								std::string ());// Default POP3 account

	std::string errMsg = newPop3Account.Validate (_passwordVerify.GetTrimmedString ());
	if (errMsg.empty ())
	{
		// POP3 dialog settings are valid - are there any changes?
		if (!currentPop3Account.IsEqual (newPop3Account))
		{
			newPop3Account.Save (emailCfg);
			changesDetected = true;
		}
	}
	else
	{
		TheOutput.Display (errMsg.c_str ());
		_isValidData = false;
		return;
	}

	SmtpAccount currentSmtpAccount (emailCfg);

	SmtpAccount newSmtpAccount (Email::GmailSmtpServer,
								newPop3Account.GetUser (),
								newPop3Account.GetPassword ());
	newSmtpAccount.SetAuthenticate (true);
	newSmtpAccount.SetUseSSL (true);
	newSmtpAccount.SetPort (Email::GmailSmtpPort);
	newSmtpAccount.SetSenderAddress (user);

	errMsg = newSmtpAccount.Validate (newPop3Account.GetPassword ()); 
	if (errMsg.empty ())
	{
		// SMTP dialog settings are valid - are there any changes?
		if (!currentSmtpAccount.IsEqual (newSmtpAccount))
		{
			newSmtpAccount.Save (emailCfg);
			changesDetected = true;
		}
		if (currentSmtpAccount.GetSenderAddress () != newSmtpAccount.GetSenderAddress ())
		{
			ConfigData & newCfg = _wizardData.GetNewConfig ();
			newCfg.SetHubId (newSmtpAccount.GetSenderAddress ());
			newCfg.SetInterClusterTransportToMe (Transport (newSmtpAccount.GetSenderAddress (), Transport::Email));
		}
	}
	else
	{
		TheOutput.Display (errMsg.c_str ());
		_isValidData = false;
	}

	if (changesDetected)
		emailMan.Refresh ();
}
