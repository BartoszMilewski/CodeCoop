#if !defined (PROJECTPROPSDLG_H)
#define PROJECTPROPSDLG_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "resource.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>

namespace Project
{
	class OptionsEx;
}

class NamedValues;

class ProjectOptionsCtrl : public PropPage::Handler
{
public:
	ProjectOptionsCtrl (Project::OptionsEx & options)
		: PropPage::Handler (IDD_PROJECT_OPTIONS_PAGE),
		  _options (options)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

	void OnSetActive (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

private:
	void InitializeControls ();

private:
	Win::CheckBox		_autoSynch;
	Win::CheckBox		_autoJoin;
	Win::CheckBox		_keepCheckedOut;
	Win::CheckBox		_checkoutNotification;
	Win::CheckBox		_autoInvite;
	Win::Edit			_projectPath;
	Win::Button			_pathBrowse;

	Project::OptionsEx &_options;
};

class ProjectDistributorCtrl : public PropPage::Handler
{
public:
	ProjectDistributorCtrl (Project::OptionsEx & options)
		: PropPage::Handler (IDD_PROJECT_DISTRIBUTOR),
		  _options (options)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

private:
	Win::CheckBox		_distributor;
	Win::CheckBox		_noBranching;
	Win::StaticText		_frame;
	Win::RadioButton	_allBcc;
	Win::RadioButton	_singleRecipient;
	Win::StaticText		_status;
	Win::StaticText		_license;
    Win::Button			_buyLicense;

	Project::OptionsEx &_options;
};

class ProjectEncryptionCtrl : public PropPage::Handler
{
public:
	ProjectEncryptionCtrl (Project::OptionsEx & options)
		: PropPage::Handler (IDD_PROJECT_ENCRYPTION),
		_options (options)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);

	void OnKillActive (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);

private:
	void InitializeControls ();
	void EnableKeyControls ();
	void DisableKeyControls ();

private:
	Win::CheckBox		_isEncrypt;
	Win::CheckBox		_useCommonKey;
	Win::Edit			_key;
	Win::Edit			_key2;
	Win::StaticText		_keyStatic;
	Win::StaticText		_key2Static;

	Project::OptionsEx &_options;
};

class ProjectOptionsHndlrSet : public PropPage::HandlerSet
{
public:
	ProjectOptionsHndlrSet (Project::OptionsEx & options);

	bool IsValidData () const { return true; }

	bool GetDataFrom (NamedValues const & source);

private:
	Project::OptionsEx &	_options;
	ProjectOptionsCtrl		_optionsPageHndlr;
	ProjectDistributorCtrl	_distributorPageHndlr;
	ProjectEncryptionCtrl	_encryptionPageHndlr;
};

#endif
