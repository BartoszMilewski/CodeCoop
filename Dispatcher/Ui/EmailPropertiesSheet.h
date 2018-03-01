#if !defined (EMAILPROPERTIESSHEET_H)
#define EMAILPROPERTIESSHEET_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "DiagFeedback.h"
#include "WizardHelp.h"
#include "EmailConfig.h"
#include "resource.h"
#include "EmailMan.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/ProgressBar.h>
#include <Ctrl/Static.h>
#include <Ctrl/Spin.h>
#include <Net/SecureSocket.h> // Is SSL supported?

namespace Win
{
	class MessagePrepro;
}
class EmailConfigData;
class ScriptProcessorConfig;
class SmtpAccount;
class Pop3Account;

class EmailCtrl : public PropPage::Handler
{
public:
	EmailCtrl (int pageId)
		: PropPage::Handler (pageId, true)	// All email controllers support context help
	{}

	void OnHelp () const throw (Win::Exception)
	{
		OpenHelp ();
	}
};

class EmailDiagnosticsCtrl : public EmailCtrl
{
	// Email Diagnostics page is displayed only when 
	// e-mail configuration is valid
public:
	EmailDiagnosticsCtrl (std::string const & myEmail, 
						  Email::Manager & emailMan, 
						  Win::MessagePrepro * msgPrepro)
		: EmailCtrl (IDD_EMAIL_DIAGNOSTICS),
		  _msgPrepro (msgPrepro),
		  _myEmail (myEmail),
		  _emailMan (emailMan)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);

private:
	void RunDiagnostics ();

	std::string		 _myEmail;
	Email::Manager & _emailMan;

	Win::Edit		 _status;
	Win::ProgressBar _progressBar;
	Win::Button		 _start;
	Win::Button		 _stop;

	DiagProgress			_diagProgress;
	Win::MessagePrepro    * _msgPrepro;
};

class EmailGeneralCtrl : public EmailCtrl
{
public:
	EmailGeneralCtrl (Email::Manager & emailMan)
		: EmailCtrl (IDD_EMAIL_GENERAL),
		  _emailMan (emailMan),
		  _emailCfg (emailMan.GetEmailConfig ())
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnKillActive (long & result) throw (Win::Exception);

private:
	void ResetControls ();
	void RetrieveData ();
	bool Validate () const;

private:
	Win::StaticText  _staticSendUsingMapi;
	Win::StaticText  _staticNoSmtp;
	Win::RadioButton _sendUsingMapi;
	Win::RadioButton _sendUsingSmtp;
	Win::Button		 _smtpAccounts;
	Win::Edit		 _maxEmailSize;

	Win::StaticText  _staticRecvUsingMapi;
	Win::StaticText  _staticNoPop3;
	Win::RadioButton _recvUsingMapi;
	Win::RadioButton _recvUsingPop3;
	Win::Button		 _pop3Accounts;
	Win::CheckBox    _autoReceive;
	Win::Edit        _autoReceivePeriod;
	Win::Spin		 _autoReceiveSpin;

	Email::Manager &	_emailMan;
	Email::RegConfig &	_emailCfg;
};

class EmailProcessingCtrl : public EmailCtrl
{
public:
	EmailProcessingCtrl (Email::Manager & emailMan)
		: EmailCtrl (IDD_EMAIL_PROCESSING),
		  _emailMan (emailMan)
	{}

	bool OnInitDialog () throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

private:
	void RetrieveData ();
	bool CheckDataAndDisplayErrors () const;

	Email::Manager & _emailMan;

	Win::Edit    _preproProgram;
	Win::Edit    _preproResult;
	Win::Edit    _postproProgram;
	Win::Edit    _postproExt;
	Win::CheckBox  _preproNeedsProjName;
	Win::CheckBox  _preproSendUnprocessed;
};

// sheet
class EmailPropertiesSheet : public PropPage::Sheet
{
public:
	EmailPropertiesSheet (Win::Dow::Handle win, 
						  Email::Manager & emailMan,
						  std::string const & myEmail,
						  Win::MessagePrepro * msgPrepro);

private:
	EmailGeneralCtrl		_generalCtrl;
	EmailDiagnosticsCtrl	_diagnosticsCtrl;
	EmailProcessingCtrl		_processingCtrl;
};

// sub-dialogs
class SmtpCtrl : public Dialog::ControlHandler
{
public:
	SmtpCtrl (SmtpAccount & data)
		: Dialog::ControlHandler (IDD_SMTP),
		  _dlgData (data),
		  _isSSLSupported (SecureSocket::IsSupported ())
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	bool      const _isSSLSupported;
	Win::Edit		_server;
	Win::Edit		_port;
	Win::CheckBox	_useSSL;
	Win::Edit		_email;
	Win::Edit		_user;
	Win::Edit		_password;
	Win::Edit		_password2;
	Win::CheckBox   _isAuthenticate;

	SmtpAccount &   _dlgData;
};

class Pop3Ctrl : public Dialog::ControlHandler
{
public:
	Pop3Ctrl (Pop3Account & data)
		: Dialog::ControlHandler (IDD_POP3),
		  _dlgData (data),
		  _isSSLSupported (SecureSocket::IsSupported ())
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	bool       const _isSSLSupported;
	Win::Edit		 _server;
	Win::Edit		 _port;
	Win::CheckBox	 _useSSL;
	Win::Edit		 _user;
	Win::Edit		 _password;
	Win::Edit		 _password2;
	Win::RadioButton _doDelete;
	Win::RadioButton _dontDelete;
	Win::Button		 _help;

	Pop3Account & _dlgData;
};

#endif
