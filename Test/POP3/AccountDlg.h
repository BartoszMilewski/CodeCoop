#if !defined (ACCOUNTDLG_H)
#define ACCOUNTDLG_H

// (c) Reliable Software 2003-2005

#include "Resource.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>

class Pop3Data
{
public:
	Pop3Data ()
		:_isValid (false)
	{}
	void Init (	std::string const & name,
				std::string const & server,
				std::string const & user,
				std::string const & password,
				std::string const & password2);
	bool IsValid () const { return _isValid; }
    void DisplayErrors (Win::Dow::Handle winOwner);
	std::string const & GetName () const { return _name; }
	std::string const & GetServer () const { return _server; }
	std::string const & GetUser () const { return _user; }
	std::string const & GetPassword () const { return _password; }
private:
	std::string _name;
	std::string _server;
	std::string _user;
	std::string _password;
	bool		_isValid;
};

class Pop3Ctrl : public Dialog::ControlHandler
{
public:
	Pop3Ctrl (Pop3Data * data)
		: Dialog::ControlHandler (IDD_POP3),
		_dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::Edit	_name;
	Win::Edit	_server;
	Win::Edit	_user;
	Win::Edit	_password;
	Win::Edit	_password2;
	Pop3Data *	_dlgData;
};

class SmtpData
{
public:
	SmtpData ()
		: _isAuthenticate (false),
		  _isValid (false)
	{}
	void Init (std::string const & server,
		std::string const & email,
		std::string const & user,
		std::string const & password,
		std::string const & password2,
		bool isAuthenticate);
	bool IsValid () const { return _isValid; }
	void DisplayErrors (Win::Dow::Handle winOwner);
	std::string const & GetServer () const { return _server; }
	std::string const & GetEmail () const { return _email; }
	std::string const & GetUser () const { return _user; }
	std::string const & GetPassword () const { return _password; }
	bool IsAuthenticate () const { return _isAuthenticate; }
private:
	std::string _server;
	std::string	_email;
	std::string _user;
	std::string _password;
	bool		_isAuthenticate;
	bool		_isValid;
};

class SmtpCtrl : public Dialog::ControlHandler
{
public:
	SmtpCtrl (SmtpData * data)
		: Dialog::ControlHandler (IDD_SMTP),
		_dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::Edit	_server;
	Win::Edit	_email;
	Win::Edit	_user;
	Win::Edit	_password;
	Win::Edit	_password2;
	Win::CheckBox _isAuthenticate;
	SmtpData *	_dlgData;
};

class AccountData
{
public:
	AccountData ()
		:_isValid (false)
	{}
	void Init ( std::string const & name,
				std::string const & email,
				std::string const & pop3Server, 
				std::string const & pop3User,
				std::string const & pop3Pass,
				std::string const & pop3Pass2,
				std::string const & smtpServer, 
				std::string const & smtpUser, 
				std::string const & smtpPass, 
				std::string const & smtpPass2, 
				bool isAuthenticate);
	bool IsValid () const { return _isValid; }
    void DisplayErrors (Win::Dow::Handle winOwner);
	std::string const & GetName () const { return _pop3Data.GetName (); }
	std::string const & GetEmail () const { return _smtpData.GetEmail (); }
	std::string const & GetPop3Server () const { return _pop3Data.GetServer (); }
	std::string const & GetSmtpServer () const { return _smtpData.GetServer (); }
	bool IsAuthenticate () const { return _smtpData.IsAuthenticate (); }
	std::string const & GetPop3User () const { return _pop3Data.GetUser (); }
	std::string const & GetPop3Pass () const { return _pop3Data.GetPassword (); }
	std::string const & GetSmtpUser () const { return _smtpData.GetUser (); }
	std::string const & GetSmtpPass () const { return _smtpData.GetPassword (); }
private:
	Pop3Data	_pop3Data;
	SmtpData	_smtpData;
	bool		_isValid;
};

class AccountCtrl : public Dialog::ControlHandler
{
public:
    AccountCtrl (AccountData * data)
		: Dialog::ControlHandler (IDD_MAIL_ACCOUNT),
		  _dlgData (data)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();
    bool OnApply () throw ();

private:
	Win::Edit		_name;
	Win::Edit		_email;
	Win::Edit		_pop3Server;
	Win::Edit		_pop3User;
	Win::Edit		_pop3Pass;
	Win::Edit		_pop3Pass2;
	Win::Edit		_smtpServer;
	Win::CheckBox	_isAuth;
	Win::Edit		_smtpUser;
	Win::Edit		_smtpPass;
	Win::Edit		_smtpPass2;
    AccountData *	_dlgData;
};

#endif
