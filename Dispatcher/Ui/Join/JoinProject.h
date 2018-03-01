#if !defined (JOINPROJECT_H)
#define JOINPROJECT_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "JoinProjectData.h"
#include "Resource.h"

#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/ComboBox.h>
#include <Ctrl/PropertySheet.h>
#include <Win/Win.h>

#include <StringOp.h>

class Catalog;
class LocalRecipientList;

class GeneralPageHndlr : public PropPage::Handler
{
public:
	GeneralPageHndlr (JoinProjectData & joinData,
					 NocaseSet const & projects,
					 NocaseMap<Transport> & hubs,
					 LocalRecipientList const & localRecipients,
					 bool iAmEmailPeer)
		: PropPage::Handler (IDD_JOIN_PROJECT, true),
		  _joinData (joinData),
		  _projects (projects),
		  _hubs (hubs),
		  _localRecipients (localRecipients),
		  _iAmEmailPeer (iAmEmailPeer)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void OnHelp () const throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	void InitRemoteClusterHubIds ();
	void ChangeRemoteClusterHubId ();

	Win::RadioButton			_thisComputer;
	Win::RadioButton			_thisCluster;
	Win::Edit					_thisClusterHubId;
	Win::RadioButton			_remoteCluster;
	Win::ComboBox				_remoteClusterHubId;
	Win::Button					_advanced;
	Win::ComboBox				_projectName;
	Win::Edit					_sourcePath;
    Win::Button					_browseSource;
    Win::Edit					_userName;
    Win::Edit					_userPhone;
    Win::CheckBox				_observer;
    JoinProjectData &			_joinData;
	NocaseSet const &			_projects;
	NocaseMap<Transport> &		_hubs;
	LocalRecipientList const &	_localRecipients;
	bool						_iAmEmailPeer;
};

class OptionsPageHndlr : public PropPage::Handler
{
public:
	OptionsPageHndlr (JoinProjectData & joinData)
		: PropPage::Handler (IDD_JOIN_PROJECT_OPTIONS, true),
		  _joinData (joinData)
	{}

	void OnCancel (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void OnHelp () const throw (Win::Exception);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw ();

private:
	Win::CheckBox		_autoSynch;
	Win::CheckBox		_autoFullSynch;
	Win::CheckBox		_keepCheckedOut;
	Win::CheckBox		_joinAsObserver;
    JoinProjectData &	_joinData;
};

class JoinProjectHndlrSet : public PropPage::HandlerSet
{
public:
	JoinProjectHndlrSet (JoinProjectData & joinData,
						 NocaseSet const & projects,
						 NocaseMap<Transport> & hubs,
						 LocalRecipientList const & localRecipients,
						 bool iAmEmailPeer);

	bool IsValidData () const { return _joinData.IsValid (); }

private:
	JoinProjectData &		_joinData;
	GeneralPageHndlr		_generalPageHndlr;
	OptionsPageHndlr		_optionsPageHndlr;
};

#endif
