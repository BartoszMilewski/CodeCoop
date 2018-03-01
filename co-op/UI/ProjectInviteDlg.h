#if !defined (PROJECTINVITEDLG_H)
#define PROJECTINVITEDLG_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2008
//------------------------------------

#include "ProjectInviteData.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>
#include <Ctrl/ComboBox.h>

class ProjectPageHandler : public PropPage::Handler
{
public:
	ProjectPageHandler (Project::InviteData & data, 
						std::vector<std::string> const & remoteHubs, 
						std::string const & myEmail);

	void OnKillActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);


private:
	void ResetSatControls (bool enable);

private:
	Win::EditReadOnly	_projectName;
	Win::Edit			_userName;
	Win::RadioButton	_peerOrHub;
	Win::RadioButton	_sat;
	Win::StaticText		_staticEmailAddr;
	Win::ComboBox		_emailAddress;
	Win::StaticText		_staticComputerName;
	Win::Edit			_computerName;

	Project::InviteData &				_pageData;
	std::vector<std::string> const &	_remoteHubs;
	std::string const					_myEmail;
};

class OptionsPageHandler : public PropPage::Handler
{
public:
	OptionsPageHandler (Project::InviteData & data); 

	void OnKillActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void OnCancel (long & result) throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);


private:
	Win::CheckBox		_observer;
	Win::CheckBox		_checkoutNotification;
	Win::CheckBox		_manualDispatch;
	Win::CheckBox		_transferHistory;
	Win::RadioButton	_myComputer;
	Win::Edit			_localFolder;
	Win::Button			_browseMyComputer;
	Win::RadioButton	_internet;
	Win::Edit			_server;
	Win::Edit			_serverFolder;
	Win::CheckBox		_anonymousLogin;
	Win::Edit			_user;
	Win::Edit			_password;

	Project::InviteData &	_pageData;
};

class ProjectInviteHandlerSet : public PropPage::HandlerSet
{
public:
	ProjectInviteHandlerSet (Project::InviteData & data, 
							 std::vector<std::string> const & remoteHubs, 
							 std::string const & myEmail);

	bool IsValidData () const { return _invitationData.IsValid (); }

	bool GetDataFrom (NamedValues const & source);

private:
	Project::InviteData &	_invitationData;
	ProjectPageHandler		_projectPageHandler;
	OptionsPageHandler		_optionsPageHandler;
};

#endif
