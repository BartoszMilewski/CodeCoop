// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "precompiled.h"
#include "WinOut.h"
#include "AccountDlg.h"
#include "EmailConfig.h"
#include "EmailAccount.h"

bool GetSMTPInfoFromUser (WinOut & out,
						  std::string & server, 
						  std::string & user, 
						  std::string & password, 
						  std::string & senderAddress,
						  unsigned long & options)
{
	Email::RegConfig emailCfg;
	SmtpAccount smtpAccount (emailCfg);
	server = smtpAccount.GetServer();
	user = smtpAccount.GetUser();
	password = smtpAccount.GetPassword();
	senderAddress = smtpAccount.GetSenderAddress();

	SmtpData data;
	// Revisit: add options
	data.Init (server, senderAddress, user, password, password, options == 1);
	SmtpCtrl ctrl (&data);
	Dialog::Modal dlg (0, ctrl);
	if (!dlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return false;
	}
	server = data.GetServer ();
	senderAddress = data.GetEmail ();
	user = data.GetUser ();
	password = data.GetPassword ();
	//Registry::SetSMTPServerInfo (server, user, password, senderAddress, 
	//							 data.IsAuthenticate () ? 1 : 0);

	out.PutLine (server.c_str ());
	out.PutLine (user.c_str ());
	out.PutLine (password.c_str ());
	out.PutLine (senderAddress.c_str ());

	return true;
}

bool GetPOP3InfoFromUser (WinOut & out,
						  std::string & server, 
						  std::string & user, 
						  std::string & password, 
						  unsigned long & options)
{
	Pop3Data data;
	std::string name;
	Email::RegConfig emailCfg;
	Pop3Account pop3Account (emailCfg, name);
	server = pop3Account.GetServer();
	user = pop3Account.GetUser();
	password = pop3Account.GetPassword();
	// Revisit: get rid of e-mail address, add options
	data.Init (server, "not@used", user, password, password);
	Pop3Ctrl ctrl (&data);
	Dialog::Modal dlg (0, ctrl);
	if (!dlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return false;
	}
	server = data.GetServer ();
	user = data.GetUser ();
	password = data.GetPassword ();
	//Registry::SetPOP3ServerInfo (data.GetName (), server, user, password, options);

	out.PutLine (server.c_str ());
	out.PutLine (user.c_str ());
	out.PutLine (password.c_str ());

	return true;
}

bool GetAccountInfoFromUser (WinOut & out, 
							 std::string & senderAddress, 
							 std::string & pop3Server, 
							 std::string & pop3User,
							 std::string & pop3Pass,
							 std::string & smtpServer, 
							 std::string & smtpUser, 
							 std::string & smtpPass, 
							 unsigned long & options)
{
	AccountData data;

	Email::RegConfig emailCfg;

	SmtpAccount smtpAccount (emailCfg);
	smtpServer = smtpAccount.GetServer();
	smtpUser = smtpAccount.GetUser();
	smtpPass = smtpAccount.GetPassword();
	senderAddress = smtpAccount.GetSenderAddress();

	std::string name;
	Pop3Account pop3Account (emailCfg, name);
	pop3Server = pop3Account.GetServer();
	pop3User = pop3Account.GetUser();
	pop3Pass = pop3Account.GetPassword();
	name = pop3Account.GetName();

	data.Init (name, senderAddress, pop3Server, pop3User, pop3Pass, pop3Pass, 
			   smtpServer, smtpUser, smtpPass, smtpPass, options == 1);

	AccountCtrl ctrl (&data);
	Dialog::Modal dlg (0, ctrl);
	if (!dlg.IsOK ())
	{
		out.PutLine ("Test aborted");
		return false;
	}
	senderAddress = data.GetEmail ();
	pop3Server = data.GetPop3Server ();
	pop3User = data.GetPop3User ();
	pop3Pass = data.GetPop3Pass ();
	smtpServer = data.GetSmtpServer ();
	smtpUser = data.GetSmtpUser ();
	smtpPass = data.GetSmtpPass ();
	options = data.IsAuthenticate () ? 1 : 0;
	//Registry::SetPOP3ServerInfo (data.GetName (), pop3Server, pop3User, pop3Pass, 0);
	//Registry::SetSMTPServerInfo (smtpServer, smtpUser, smtpPass, senderAddress, options);

	out.PutLine (data.GetName ().c_str ());
	out.PutLine (senderAddress.c_str ());
	out.PutLine (pop3Server.c_str ());
	out.PutLine (pop3User.c_str ());
	out.PutLine (pop3Pass.c_str ());
	out.PutLine (smtpServer.c_str ());
	out.PutLine (smtpUser.c_str ());
	out.PutLine (smtpPass.c_str ());
	if (options == 1)
	{
		out.PutLine ("SMTP server requires authentication");
		out.PutLine (smtpUser.c_str ());
		out.PutLine (smtpPass.c_str ());
	}
	else
		out.PutLine ("SMTP server does not require authentication.");

	return true;
}
