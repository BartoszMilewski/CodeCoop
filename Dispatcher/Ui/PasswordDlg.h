#if !defined (PASSWORDDLG_H)
#define PASSWORDDLG_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>

class EmailPasswordCtrl : public Dialog::ControlHandler
{
public:
	EmailPasswordCtrl ();

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	std::string const & GetPop3Password () const { return _pop3Password; }
	std::string const & GetSmtpPassword () const { return _smtpPassword; }
	bool UseSmtpPassword () const { return _useSmtpPassword; }

private:
	Win::EditReadOnly	_emailAddress;
	Win::EditReadOnly	_userName;
	Win::Edit			_pop3PasswordEdit;
	Win::Edit			_pop3PasswordVerify;
	Win::Edit			_smtpPasswordEdit;
	Win::Edit			_smtpPasswordVerify;
	Win::CheckBox		_smtpUsesPassword;
	Win::RadioButton	_sameAsPop3;
	Win::RadioButton	_different;

	std::string			_pop3Password;
	bool				_useSmtpPassword;
	std::string			_smtpPassword;
};


#endif
