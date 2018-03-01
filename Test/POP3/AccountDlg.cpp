// ---------------------------------
// (c) Reliable Software 2003 - 2005
//----------------------------------
#include "precompiled.h"
#include "AccountDlg.h"

void Pop3Data::Init (std::string const & name,
					 std::string const & server,
					 std::string const & user,
					 std::string const & password,
					 std::string const & password2)
{
	_name = name;
	_server = server;
	_user = user;
	_password = password;
	_isValid = !_name.empty() && !server.empty () && !user.empty () && !password.empty ()
		&& (password == password2);
}

void Pop3Data::DisplayErrors (Win::Dow::Handle winOwner)
{
	std::string msg;
	if (_name.empty ())
		msg += "Specify name for your account";
	else if (_server.empty ())
		msg += "Specify POP3 server";
	else if (_user.empty ())
		msg += "Specify POP3 user name";
	else if (_password.empty ())
		msg += "Specify password";
	else
	{
		msg += "Passwords don't match";
		_password.clear ();
	}
	// Revisit: use TheOutput
	::MessageBox (winOwner.ToNative (), msg.c_str (), "Invalid account data", MB_ICONERROR | MB_OK);
}

bool Pop3Ctrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_name.Init (GetWindow (), IDC_NAME);
	_name.SetText (_dlgData->GetName ().c_str ());
	_server.Init (GetWindow (), IDC_EDIT1);
	_server.SetText (_dlgData->GetServer ().c_str ());
	_user.Init (GetWindow (), IDC_EDIT2);
	_user.SetText (_dlgData->GetUser ().c_str ());
	_password.Init (GetWindow (), IDC_EDIT3);
	_password.SetText (_dlgData->GetPassword ().c_str ());
	_password2.Init (GetWindow (), IDC_EDIT4);
	_password2.SetText (_dlgData->GetPassword ().c_str ());
	return true;
}

bool Pop3Ctrl::OnApply () throw ()
{
	_dlgData->Init (_name.GetTrimmedString (),
					_server.GetTrimmedString (),
					_user.GetTrimmedString (), 
					_password.GetTrimmedString (), 
					_password2.GetTrimmedString ());
	if (_dlgData->IsValid ())
	{
		EndOk ();
	}
	else
	{
		_dlgData->DisplayErrors (GetWindow ());
		_password.Clear ();
		_password2.Clear ();
	}
	return true;
}

void SmtpData::Init (std::string const & server,
					 std::string const & email,
					 std::string const & user,
					 std::string const & password,
					 std::string const & password2,
					 bool isAuthenticate)
{
	_server = server;
	_email = email;
	_user = user;
	_password = password;
	_isAuthenticate = isAuthenticate;
	_isValid = !server.empty () && !_email.empty () && 
				(!_isAuthenticate || !user.empty () && !password.empty () && (password == password2));
}

void SmtpData::DisplayErrors (Win::Dow::Handle winOwner)
{
	std::string msg;
	if (_server.empty ())
		msg += "Specify SMTP server";
	else if (_email.empty ())
		msg += "Specify your email address";
	else if (_user.empty ())
		msg += "Specify POP3 user name";
	else if (_password.empty ())
		msg += "Specify password";
	else
	{
		msg += "Passwords don't match";
		_password.clear ();
	}
	// Revisit: use TheOutput
	::MessageBox (winOwner.ToNative (), msg.c_str (), "Invalid account data", MB_ICONERROR | MB_OK);
}

bool SmtpCtrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_server.Init (GetWindow (), IDC_EDIT1);
	_server.SetText (_dlgData->GetServer ().c_str ());
	_email.Init (GetWindow (), IDC_EMAIL);
	_email.SetText (_dlgData->GetEmail ().c_str ());
	_user.Init (GetWindow (), IDC_EDIT2);
	_user.SetText (_dlgData->GetUser ().c_str ());
	_password.Init (GetWindow (), IDC_EDIT3);
	_password.SetText (_dlgData->GetPassword ().c_str ());
	_password2.Init (GetWindow (), IDC_EDIT4);
	_password2.SetText (_dlgData->GetPassword ().c_str ());
	_isAuthenticate.Init (GetWindow (), IDC_AUTH);
	if (_dlgData->IsAuthenticate ())
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
	}
	return false;
}

bool SmtpCtrl::OnApply () throw ()
{
	_dlgData->Init (
		_server.GetTrimmedString (),
		_email.GetTrimmedString (),
		_user.GetTrimmedString (), 
		_password.GetTrimmedString (), 
		_password2.GetTrimmedString (),
		_isAuthenticate.IsChecked ());
	if (_dlgData->IsValid ())
	{
		EndOk ();
	}
	else
	{
		_dlgData->DisplayErrors (GetWindow ());
		_password.Clear ();
		_password2.Clear ();
	}
	return true;
}

void AccountData::Init (std::string const & name,
						std::string const & email,
						std::string const & pop3Server, 
						std::string const & pop3User,
						std::string const & pop3Pass,
						std::string const & pop3Pass2,
						std::string const & smtpServer, 
						std::string const & smtpUser, 
						std::string const & smtpPass, 
						std::string const & smtpPass2,
						bool isAuthenticate)
{
	_pop3Data.Init (name, pop3Server, pop3User, pop3Pass, pop3Pass2);
	_smtpData.Init (smtpServer, email, smtpUser, smtpPass, smtpPass2, isAuthenticate);
	_isValid = _pop3Data.IsValid () && _smtpData.IsValid ();
}

void AccountData::DisplayErrors (Win::Dow::Handle winOwner)
{
	if (!_pop3Data.IsValid ())
		_pop3Data.DisplayErrors (winOwner);
	if (!_smtpData.IsValid ())
		_smtpData.DisplayErrors (winOwner);
}

bool AccountCtrl::OnInitDialog () throw (Win::Exception)
{
	GetWindow ().CenterOverScreen ();
	_name.Init (GetWindow (), IDC_NAME);
	_email.Init (GetWindow (), IDC_EMAIL);
	_pop3Server.Init (GetWindow (), IDC_EDIT_POP3);
	_smtpServer.Init (GetWindow (), IDC_EDIT_SMTP);
	_isAuth.Init (GetWindow (), IDC_AUTH);
	_pop3User.Init (GetWindow (), IDC_POP3_USER);
	_pop3Pass.Init (GetWindow (), IDC_POP3_PASS);
	_pop3Pass2.Init (GetWindow (), IDC_POP3_PASS2);
	_smtpUser.Init (GetWindow (), IDC_SMTP_USER);
	_smtpPass.Init (GetWindow (), IDC_SMTP_PASS);
	_smtpPass2.Init (GetWindow (), IDC_SMTP_PASS2);

	_name.SetText (_dlgData->GetName ().c_str ());
	_email.SetText (_dlgData->GetEmail ().c_str ());
	_pop3Server.SetText (_dlgData->GetPop3Server ().c_str ());
	_smtpServer.SetText (_dlgData->GetSmtpServer ().c_str ());
	_pop3User.SetText (_dlgData->GetPop3User ().c_str ());
	_pop3Pass.SetText (_dlgData->GetPop3Pass ().c_str ());
	_pop3Pass2.SetText (_dlgData->GetPop3Pass ().c_str ());
	_smtpUser.SetText (_dlgData->GetSmtpUser ().c_str ());
	_smtpPass.SetText (_dlgData->GetSmtpPass ().c_str ());
	_smtpPass2.SetText (_dlgData->GetSmtpPass ().c_str ());
	if (_dlgData->IsAuthenticate ())
	{
		_isAuth.Check ();
	}
	else
	{
		_smtpUser.SetReadonly (true);
		_smtpPass.SetReadonly (true);
		_smtpPass2.SetReadonly (true);
	}
	return true;
}

bool AccountCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	switch (id)
	{
	case IDC_AUTH:
		if (Win::SimpleControl::IsClicked (notifyCode))
		 if (_isAuth.IsChecked ())
		 {
			 _smtpUser.SetReadonly (false);
			 _smtpPass.SetReadonly (false);
			 _smtpPass2.SetReadonly (false);
		 }
		 else
		 {
			 _smtpUser.SetReadonly (true);
			 _smtpPass.SetReadonly (true);
			 _smtpPass2.SetReadonly (true);
		 }
	};
	return false;
}

bool AccountCtrl::OnApply () throw ()
{
	_dlgData->Init (_name.GetTrimmedString (), 
					_email.GetTrimmedString (),
					_pop3Server.GetTrimmedString (),
					_pop3User.GetTrimmedString (), 
					_pop3Pass.GetTrimmedString (), 
					_pop3Pass.GetTrimmedString (),
					_smtpServer.GetTrimmedString (),
					_smtpUser.GetTrimmedString (), 
					_smtpPass.GetTrimmedString (), 
					_smtpPass2.GetTrimmedString (),
					_isAuth.IsChecked ());
	if (_dlgData->IsValid ())
	{
		EndOk ();
	}
	else
	{
		_dlgData->DisplayErrors (GetWindow ());
		_pop3Pass.Clear ();
		_pop3Pass2.Clear ();
		_smtpPass.Clear ();
		_smtpPass2.Clear ();
	}
	return true;
}

