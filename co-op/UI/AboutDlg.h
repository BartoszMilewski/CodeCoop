#if !defined (ABOUTDLG_H)
#define ABOUTDLG_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include "Resource.h"

#include <Ctrl/Static.h>
#include <Ctrl/Edit.h>
#include <Win/Dialog.h>

class MemberDescription;

class HelpAboutData
{
public:
	HelpAboutData (MemberDescription const * currentUser,
				   std::string const & userLicense,
				   std::string const & projectName,
				   char const * projectPath);
    char const * GetUser () const { return _userName.c_str (); }
    char const * GetUserEmail () const { return _userEmail.c_str (); }
    char const * GetUserPhone () const { return _userPhone.c_str (); }
	char const * GetUserLicense () const { return _userLicense.c_str (); }
    char const * GetProjectName () const { return _projectName.c_str (); }
    char const * GetProjectPath () const { return _projectPath.c_str (); }

private:
    std::string _userName;
    std::string _userEmail;
    std::string _userPhone;
	std::string _userLicense;
    std::string _projectName;
    std::string _projectPath;
};

class HelpAboutCtrl : public Dialog::ControlHandler // Dialog::ControlHandler
{
public:
    HelpAboutCtrl (HelpAboutData * data)
		: Dialog::ControlHandler (IDD_ABOUT),
		  _dlgData (data)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::StaticText _copyright;
    Win::StaticText _userName;
    Win::StaticText _userEmail;
    Win::StaticText _userPhone;
	Win::StaticText _userLicense;
    Win::StaticText _projectName;
    Win::StaticText _projectPath;
    HelpAboutData * _dlgData;
};

class LicenseDlgData
{
public:
	LicenseDlgData (Win::Instance inst);
	char const * GetText () const throw () { return _text; }
private:
	char const * _text;
};

class LicenseDlgCtrl : public Dialog::ControlHandler
{
public:
    LicenseDlgCtrl (LicenseDlgData * data)
        : Dialog::ControlHandler (IDD_LICENSE_AGREEMENT),
		  _dlgData (data)
    {}

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply ();

private:
	Win::EditReadOnly   _licenseText;

    LicenseDlgData *	_dlgData;
};

#endif
