// ----------------------------------
// (c) Reliable Software, 2005 - 2008
// ----------------------------------

#include <precompiled.h>
#include "EmailAccount.h"
#include "EmailConfig.h"

#include <Mail/EmailAddress.h>
#include <Sys/RegKey.h>

unsigned long GetVerifyTimeout (RegKey::Handle key)
{
	unsigned long timeout = 0;
	key.GetValueLong ("timeout", timeout);
	if ((timeout < Email::Account::MinServerTimeout) || (timeout > Email::Account::MaxServerTimeout))
		timeout = Email::Account::DefaultServerTimeout;

	return timeout;
}

short GetVerifyPort (RegKey::Handle key, short defaultPort)
{
	unsigned long port = 0;
	if (key.GetValueLong ("port", port) && port > 0)
		return (short)port;
	else
		return defaultPort;
}

bool ServerAccount::IsEqual (ServerAccount const & account) const
{
	return Email::Account::IsEqual (account) &&
		   _timeout == account.GetTimeout () &&
		   _port == account.GetPort () &&
		   _server == account.GetServer () &&
		   _user == account.GetUser () &&
		   _password == account.GetPassword ();
}

Pop3Account::Pop3Account (Email::RegConfig const & emailCfg, std::string const & accountName, bool getPasswd)
	: ServerAccount (accountName, DefaultPop3Port)
{
	_technology.SetPop3 ();
	SetDeleteNonCoopMsg (true);	// Default - Code Co-op dedicated account

	if (!emailCfg.IsPop3RegKey ())
		return;

	RegKey::AutoHandle pop3Key = emailCfg.GetPop3RegKey ();
	if (_name.empty ())
	{
		Load (pop3Key, getPasswd);
	}
	else
	{
		RegKey::Check check (pop3Key, _name);
		if (!check.Exists ())
			return;

		RegKey::ReadOnly subkey (pop3Key, _name);
		Load (subkey, getPasswd);
	}
}

Pop3Account::Pop3Account (std::string const & server,
						  std::string const & user,
						  std::string const & password,
						  bool useSSL,
						  std::string const & accountName)
	: ServerAccount (accountName,
					 server,
					 user,
					 password,
					 useSSL ? DefaultSSLPop3Port : DefaultPop3Port)
{
	_technology.SetPop3 ();
	SetUseSSL (useSSL);
	SetDeleteNonCoopMsg (true);	// Default - Code Co-op dedicated account
}

void Pop3Account::Reset (Pop3Account const & account)
{
	_isValid = account._isValid;
	_timeout = account.GetTimeout ();
	SetServer (account.GetServer ());
	SetUser (account.GetUser ());
	SetPassword (account.GetPassword ());
	SetPort (account.GetPort ());
	_options.init (account._options.to_ulong ());
}

bool Pop3Account::IsEqual (Pop3Account const & account) const
{
	return ServerAccount::IsEqual (account) &&
		   _options.to_ulong () == account._options.to_ulong ();
}

void Pop3Account::Load (RegKey::Handle key, bool getPasswd)
{
	_server	= key.GetStringVal ("server");
	_user	= key.GetStringVal ("user");
	unsigned long options = 0;
	key.GetValueLong ("options", options);
	_options.init (options);
	_timeout = GetVerifyTimeout (key);
	_port = GetVerifyPort (key, DefaultPop3Port);
	Assert (_port > 0);
	if (getPasswd)
		key.GetStringDecrypt ("password", _password);
	_isValid = Validate (_password).empty ();
}

void Pop3Account::Save (Email::RegConfig const & emailCfg) const
{
	RegKey::AutoHandle pop3Key = emailCfg.GetPop3RegKey ();
	if (_name.empty ())
	{
		Save (pop3Key);
	}
	else
	{
		RegKey::New subkey (pop3Key, _name);
		Save (subkey);
	}
}

void Pop3Account::Save (RegKey::Handle key) const
{
	key.SetValueString ("server", _server);
	key.SetValueString ("user", _user);
	key.SetValueLong ("options", _options.to_ulong ());
	key.SetValueLong ("timeout", _timeout);
	key.SetValueLong ("port", _port);
	key.SetStringEncrypt ("password", _password);
}

std::string Pop3Account::Validate (std::string const & passwordVerify) const
{
	if (_server.empty ())
		return "Specify POP3 server.";
	else if (_port <= 0)
		return "Specify valid port number.";
	else if (_user.empty ())
		return "Specify POP3 user name.";
	else if (_password.empty ())
		return "Specify password.";
	else if (_password != passwordVerify)
		return "POP3 passwords don't match.";

	return std::string ();
}

void Pop3Account::Dump (std::ostream & out) const
{
	out << "*Pop3 account settings:" << std::endl;
	out << "**Server: " << _server << std::endl;
	out << "**Port: " << _port << std::endl;
	out << "**User: " << _user << std::endl;
	if (UseSSL ())
		out << "**SSL is on." << std::endl;
	else
		out << "**SSL is off." << std::endl;
	if (IsDeleteNonCoopMsg ())
		out << "**Non-coop messages will be removed from server." << std::endl;
	else
		out << "**Non-coop messages will be left untouched on server." << std::endl;
}

SmtpAccount::SmtpAccount (Email::RegConfig const & emailCfg, bool getPasswd)
	: ServerAccount (std::string (), DefaultSmtpPort)
{
	_technology.SetSmtp ();
	if (!emailCfg.IsSmtpRegKey ())
		return;

	RegKey::AutoHandle smtpKey = emailCfg.GetSmtpRegKey ();

	_server	= smtpKey.GetStringVal ("server");
	_user	= smtpKey.GetStringVal ("user");
	_senderAddress = smtpKey.GetStringVal ("email");
	_senderName = "Code Co-op";
	unsigned long options = 0;
	smtpKey.GetValueLong ("options", options);
	_options.init (options);
	_timeout = GetVerifyTimeout (smtpKey);
	_port = GetVerifyPort (smtpKey, DefaultSmtpPort);
	Assert (_port > 0);
	if (getPasswd)
		smtpKey.GetStringDecrypt ("password", _password);
	_isValid = Validate (_password).empty ();
}

SmtpAccount::SmtpAccount (std::string const & server,
						  std::string const & user,
						  std::string const & password)
	: ServerAccount (std::string (),
					 server,
					 user,
					 password,
					 DefaultSmtpPort)
{
	_technology.SetSmtp ();
	SetSenderName ("Code Co-op");
}

bool SmtpAccount::IsEqual (SmtpAccount const & account) const
{
	return ServerAccount::IsEqual (account) &&
		   _senderName == account.GetSenderName () &&
		   _senderAddress == account.GetSenderAddress () &&
		   _options.to_ulong () == account._options.to_ulong ();
}

void SmtpAccount::Reset (SmtpAccount const & account)
{
	_isValid = account._isValid;
	_timeout = account.GetTimeout ();
	SetServer (account.GetServer ());
	SetUser (account.GetUser ());
	SetPassword (account.GetPassword ());
	SetPort (account.GetPort ());
	SetSenderName (account.GetSenderName ());
	SetSenderAddress (account.GetSenderAddress ());
	_options.init (account._options.to_ulong ());
}

void SmtpAccount::Save (Email::RegConfig const & emailCfg) const
{
	RegKey::AutoHandle smtpKey = emailCfg.GetSmtpRegKey ();
	smtpKey.SetValueString ("email", _senderAddress);
	smtpKey.SetValueString ("server", _server);
	smtpKey.SetValueLong ("timeout", _timeout);
	smtpKey.SetValueLong ("port", _port);
	smtpKey.SetValueLong ("options", _options.to_ulong ());
	if (IsAuthenticate ())
	{
		smtpKey.SetValueString ("user", _user);
		smtpKey.SetStringEncrypt ("password", _password);
	}
}

std::string SmtpAccount::Validate (std::string const & passwordVerify) const
{
	if (_senderAddress.empty ())
		return "Specify your email address.";
	else if (!Email::IsValidAddress (_senderAddress))
		return "Email address is incorrect.";
	else if (_server.empty ())
		return "Specify SMTP server.";
	else if (_port <= 0)
		return "Specify valid port number.";
	else if (IsAuthenticate ())
	{
		if (_user.empty ())
			return "Specify SMTP user name.";
		else if (_password.empty ())
			return "Specify password.";
		else if (_password != passwordVerify)
			return "SMTP passwords don't match.";
	}
	return std::string ();
}

void SmtpAccount::Dump (std::ostream & out) const
{
	out << "*Smtp account settings:" << std::endl;
	out << "**Sender address: " << _senderAddress << std::endl; 
	out << "**Server: " << _server << std::endl; 
	out << "**Port: " << _port << std::endl; 
	if (UseSSL ())
		out << "**SSL is on." << std::endl;
	else
		out << "**SSL is off." << std::endl;
	if (IsAuthenticate ())
	{
		out << "**Server requires authentication: Yes." << std::endl; 
		out << "**User: " << _user << std::endl; 
	}
	else
		out << "**Server requires authentication: No." << std::endl; 
}
