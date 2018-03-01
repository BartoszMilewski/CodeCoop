#if !defined (LICENSEDLG_H)
#define LICENSEDLG_H
// ----------------------------------
// (c) Reliable Software 1998 -- 2005
// ----------------------------------

#include "Resource.h"

#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

class LicenseData
{
public:
	LicenseData (bool isLicensed, std::string const & status);

	bool SetNewLicense (std::string const & licensee, std::string const & key);
	void SetNagging (bool flag) { _nag = flag; }

	std::string const & GetNewLicense () const { return _newLicense; }
	char const * GetStatus () const { return _status.c_str (); }
	bool IsNewLicense () const { return !_newLicense.empty (); }
	bool IsNoNagging () const { return !_nag; }
	bool IsLicensed () const { return _isLicensed; }

private:
    std::string	_newLicense;
	std::string _status;
	bool		_isLicensed;
	bool		_nag;
};

class LicenseCtrl : public Dialog::ControlHandler
{
public:
    LicenseCtrl (LicenseData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();
	bool OnCancel () throw ();

private:
	Win::Button		_purchase;
	Win::Button     _register;
	Win::Button     _evaluate;
    Win::Edit       _licensee;
    Win::Edit       _key;
	Win::StaticText	_status;
	Win::CheckBox	_dontNag;
    LicenseData *	_dlgData;
};

#endif
