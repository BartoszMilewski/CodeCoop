#if !defined (COLLABORATIONSHEET_H)
#define COLLABORATIONSHEET_H
// ----------------------------------
// (c) Reliable Software, 2002 - 2007
// ----------------------------------

#include "WizardHelp.h"
#include "resource.h"
#include "ConfigData.h"

#include <Ctrl/PropertySheet.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Bit.h>

class ConfigDlgData;

class CollaborationCtrl : public PropPage::Handler
{
public:
	CollaborationCtrl (int pageId, ConfigDlgData & data)
		: PropPage::Handler (pageId, true),	// All collaboration controllers support context help
		  _data (data)
	{}

	void OnHelp () const throw (Win::Exception) { OpenHelp (); }

protected:
	ConfigDlgData &					_data;
	BitFieldMask<ConfigData::Field>	_modified;
};

// page controllers

// Simple configurations: Standalone or Peer page

class SimpleCollaborationCtrl : public CollaborationCtrl
{
public:
	SimpleCollaborationCtrl (ConfigDlgData & data)
		: CollaborationCtrl (IDD_COLLABORATION_SIMPLE, data)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);

private:
	Win::RadioButton _standalone;
	Win::RadioButton _peer;
	Win::Edit		 _myEmail;
	Win::Button		 _emailProperties;
};

// Network Hub page

class HubCollaborationCtrl : public CollaborationCtrl
{
public:
	HubCollaborationCtrl (ConfigDlgData & data)
		: CollaborationCtrl (IDD_COLLABORATION_HUB, data)
	{}

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void Enable (bool enable);

private:
	Win::RadioButton	_hub;
	Win::Edit			_email;
	Win::StaticText		_hubShare;
	Win::CheckBox		_emailUsage;
	Win::Button			_emailProperties;
	Win::Button			_myNetworkProperties;
};

// Network Satellite page

class SatCollaborationCtrl : public CollaborationCtrl
{
public:
	SatCollaborationCtrl (ConfigDlgData & data)
		: CollaborationCtrl (IDD_COLLABORATION_SAT, data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void Enable (bool enable);

private:
	Win::RadioButton	_satellite;

	Win::Edit			_hubEmail;
	Win::Edit			_hubShare;
	Win::Button			_hubNetworkProperties;

	Win::StaticText		_satShare;
	Win::Button			_satNetworkProperties;
};

class TemporaryHubCollaborationCtrl : public CollaborationCtrl
{
public:
	TemporaryHubCollaborationCtrl (ConfigDlgData & data)
		: CollaborationCtrl (IDD_COLLABORATION_TEMP_HUB, data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void Enable (bool enable);

private:
	Win::RadioButton	_tempHub;
	Win::StaticText		_email;
	Win::StaticText		_hubShare;
	Win::StaticText		_myEmail;
	Win::Button			_emailProperties;
};

class RemoteSatCollaborationCtrl : public CollaborationCtrl
{
public:
	RemoteSatCollaborationCtrl (ConfigDlgData & data)
		: CollaborationCtrl (IDD_COLLABORATION_REMOTE_SAT, data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	void OnSetActive (long & result) throw (Win::Exception);
	void OnApply (long & result) throw (Win::Exception);
	void Enable (bool enable);

private:
	Win::RadioButton _remoteSat;
	Win::StaticText	 _hubEmail;
	Win::Edit		 _myEmail;
	Win::Button		 _emailProperties;
};

// aux dialog
class HubIdAndRemoteTransportCtrl : public Dialog::ControlHandler
{
public:
	HubIdAndRemoteTransportCtrl (
		std::string const & currentHubId,
		std::string const & hubRemoteTransport,
		std::string		  &	newHubId)
	: Dialog::ControlHandler (IDD_HUB_ID_TRANSPORT),
	  _currentHubId (currentHubId),
	  _hubRemoteTransport (hubRemoteTransport),
      _newHubId (newHubId)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::EditReadOnly _currentHubIdEdit;
	Win::EditReadOnly _hubRemoteTransportEdit;
	Win::Edit         _newHubIdEdit;

	std::string const & _currentHubId;
	std::string const & _hubRemoteTransport;
	std::string		  &	_newHubId;
};

// sheet

class CollaborationSheet : public PropPage::Sheet
{
public:
	CollaborationSheet (Win::Dow::Handle win, ConfigDlgData & data);
	bool Execute ();

private:
	Win::Dow::Handle				_win;
	ConfigDlgData &					_cfg;
	SimpleCollaborationCtrl			_simpleCtrl;
	HubCollaborationCtrl			_networkHubCtrl;
	SatCollaborationCtrl			_networkSatCtrl;
	TemporaryHubCollaborationCtrl	_tempHubCtrl;
	RemoteSatCollaborationCtrl		_remoteSatCtrl;
};

#endif
