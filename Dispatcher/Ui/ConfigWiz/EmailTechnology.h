#if !defined (EMAILTECHNOLOGY_H)
#define EMAILTECHNOLOGY_H
// ----------------------------------
// (c) Reliable Software, 2006 - 2008
// ----------------------------------

#include "BaseWizardHandler.h"
#include "ConfigDlgData.h"
#include "EmailConfig.h"
#include "resource.h"

#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>

class ConfigDlgData;

class EmailSelectionHandler : public BaseWizardHandler
{
public:
	EmailSelectionHandler (ConfigDlgData & config, unsigned pageId, bool isFirst = false)
		: BaseWizardHandler (config, 
							pageId, 
							isFirst ? PropPage::Wiz::Next : PropPage::Wiz::NextBack)
	{}
	bool OnInitDialog () throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	bool Validate () const;

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	long  ChooseNextPage () const throw (Win::Exception);

protected:
	Win::RadioButton	_emailProgram;
	Win::RadioButton	_pop3Smtp;
	Win::RadioButton	_gmail;
};

class HubEmailSelectionHandler : public EmailSelectionHandler
{
public:
	HubEmailSelectionHandler (ConfigDlgData & config, unsigned pageId)
		: EmailSelectionHandler (config, pageId, false)
	{}

	bool OnInitDialog () throw (Win::Exception);

private:
	Win::StaticText	_hubShare;
};

class EmailPop3SmtpHandler : public BaseWizardHandler
{
public:
	EmailPop3SmtpHandler (ConfigDlgData & config, unsigned pageId);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

	bool Validate () const;

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	long  ChooseNextPage () const throw (Win::Exception) { return IDD_WIZARD_DIAGNOSTICS; }

private:
	void LoadCtrls ();
	void ResetAuthenticateCtrls ();

private:
	Win::Edit			_pop3User;
	Win::Edit			_pop3Password;
	Win::Edit			_pop3PasswordVerify;
	Win::Edit			_pop3Server;
	Win::Edit			_emailAddress;
	Win::Edit			_smtpServer;
	Win::CheckBox		_isAuthenticate;
	Win::CheckBox		_isSameAsPop3;
	Win::Edit			_smtpUser;
	Win::Edit			_smtpPassword;
	Win::Edit			_smtpPasswordVerify;
	Win::Button			_advancedPop3Smpt;

	bool				_isValidData;
};

class EmailGmailHandler : public BaseWizardHandler
{
public:
	EmailGmailHandler (ConfigDlgData & config, unsigned pageId);

	bool OnInitDialog () throw (Win::Exception);
	bool Validate () const;

protected:
	void RetrieveData (bool acceptPage) throw (Win::Exception);
	long  ChooseNextPage () const throw (Win::Exception) { return IDD_WIZARD_DIAGNOSTICS; }

private:
	Win::Edit	_user;
	Win::Edit	_password;
	Win::Edit	_passwordVerify;

	bool		_isValidData;
};

#endif
