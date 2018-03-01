// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "precompiled.h"
#include "CollaborationSheet.h"
#include "ConfigDlgData.h"
#include "EmailPropertiesSheet.h"
#include "EmailOptionsSheet.h"
#include "NetworkPropertiesDlg.h"
#include "OutputSink.h"
#include "EmailMan.h"
#include "Prompter.h"

#include <Mail/EmailAddress.h>

void DisplayEmailCfgErr ()
{
	TheOutput.Display ("The e-mail configuration is invalid.\n"
		"Use Email Options to correct the configuration.");
}

CollaborationSheet::CollaborationSheet (Win::Dow::Handle win, ConfigDlgData & data)
	: PropPage::Sheet (win, "Code Co-op Dispatcher Collaboration Settings"),
	  _win (win),
	  _cfg (data),
	  _simpleCtrl (data),
	  _networkHubCtrl (data),
	  _networkSatCtrl (data),
	  _tempHubCtrl (data),
	  _remoteSatCtrl (data)
{
	SetNoApplyButton ();
	SetNoContextHelp ();
	AddPage (_simpleCtrl,"Simple");
	AddPage (_networkHubCtrl, "Network Hub");
	AddPage (_networkSatCtrl, "Network Satellite");
	AddPage (_tempHubCtrl, "Off-Site Hub");
	AddPage (_remoteSatCtrl, "Off-Site Satellite");
}

bool CollaborationSheet::Execute ()
{
    long startPage = 0; // Standalone or Peer
	Topology topology = _cfg.GetNewConfig ().GetTopology ();
	if (topology.IsHub ())
		startPage = 1;
	else if (topology.IsSatellite ())
		startPage = 2;
	else if (topology.IsTemporaryHub ())
		startPage = 3;
	else if  (topology.IsRemoteSatellite ())
		startPage = 4;

	SetStartPage (startPage);
	Display ();

	bool isChanged = _cfg.AnalyzeChanges ();
	if (isChanged)
	{
		if (_cfg.GetNewTopology ().IsTemporaryHub () && !_cfg.WasProxy ())
			_cfg.GetNewConfig ().SetAskedToStayOffSiteHub (false);
	}
	return isChanged;
}

//------------------------------------------------
// Simple collaboration: Standalone and Email Peer
//------------------------------------------------
bool SimpleCollaborationCtrl::OnInitDialog () throw (Win::Exception)
{
	_standalone.Init (GetWindow (), IDC_STANDALONE);
	_peer.Init (GetWindow (), IDC_PEER);
	_myEmail.Init (GetWindow (), IDC_MY_EMAIL);
	_emailProperties.Init (GetWindow (), IDC_EMAIL_PROPERTIES);

	_modified.Set (ConfigData::bitTopologyCfg);
	_modified.Set (ConfigData::bitHubId);
	_modified.Set (ConfigData::bitInterClusterTransportToMe);

	Win::StaticImage stc;
	stc.Init( GetWindow (), IDC_STANDALONE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_PEER_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	
	return true;
}

void SimpleCollaborationCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData const & cfg = _data.GetNewConfig ();

	_myEmail.SetText (cfg.GetInterClusterTransportToMe ().GetRoute ().c_str ());
	if (cfg.GetTopology ().IsStandalone ())
	{
		_standalone.Check ();
		_myEmail.Disable ();
		_emailProperties.Disable ();
	}
	else if (cfg.GetTopology ().IsPeer ())
	{
		_peer.Check ();
		_myEmail.Enable ();
		_emailProperties.Enable ();
	}
	else
	{
		_standalone.UnCheck ();
		_peer.UnCheck ();
		_myEmail.Disable ();
		_emailProperties.Disable ();
	}
}

bool SimpleCollaborationCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_STANDALONE:
		_data.GetNewConfig ().MakeStandalone ();
		_myEmail.Disable ();
		_emailProperties.Disable ();
		break;
	case IDC_PEER:
		_data.GetNewConfig ().MakePeer ();
		_myEmail.Enable ();
		_emailProperties.Enable ();
		break;
	case IDC_EMAIL_PROPERTIES:
		{
			Email::Manager & emailMan = _data.GetEmailMan ();
			if (!emailMan.IsInTransaction ())
				emailMan.BeginEdit ();
			Email::RunOptionsSheet (_myEmail.GetString (), 
									emailMan, 
									GetWindow (), 
									_data.GetMsgPrepro ());
		}
		break;
	}
	return false;
}

void SimpleCollaborationCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData & cfg = _data.GetNewConfig ();
	if (!cfg.GetTopology ().IsPeer () && !cfg.GetTopology ().IsStandalone ())
		return;

	if (cfg.GetTopology ().IsPeer ())
	{
		// Configure as peer
		Transport newTransport (_myEmail.GetString ());
		if (!newTransport.IsEmail ())
		{
			TheOutput.Display ("Please specify a valid e-mail address");
			result = -1;
		}
		else if (!_data.GetEmailMan ().IsValid ())
		{
			DisplayEmailCfgErr ();
			result = -1;
		}
		cfg.SetHubId (newTransport.GetRoute ());
		cfg.SetInterClusterTransportToMe (newTransport);
	}
	
	if (result == 0)
		_data.AcceptChangesTo (_modified);
}

//------------
// Network Hub
//------------

bool HubCollaborationCtrl::OnInitDialog () throw (Win::Exception)
{
	_hub.Init (GetWindow (), IDC_HUB);
	_email.Init (GetWindow (), IDC_HUB_EMAIL);
	_hubShare.Init (GetWindow (), IDC_HUB_SHARE);
	_emailUsage.Init (GetWindow (), IDC_EMAIL_USAGE);
	_emailProperties.Init (GetWindow (), IDC_EMAIL_PROPERTIES);
	_myNetworkProperties.Init (GetWindow (), IDC_MY_NETWORK_PROPERTIES);

	_modified.Set (ConfigData::bitTopologyCfg);
	_modified.Set (ConfigData::bitHubId);
	_modified.Set (ConfigData::bitInterClusterTransportToMe);
	_modified.Set (ConfigData::bitActiveIntraClusterTransportToMe);

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SHARE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

void HubCollaborationCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData const & cfg = _data.GetNewConfig ();

	if (cfg.GetTopology ().IsHub ())
		Enable (true);
	else
		Enable (false);
}

// Note: there is some unpleasant confusion about the meaning of UseEmail
// In terms of topology, alternative remote transports count towards UseEmail,
// In user interface, UseEmail means just that: email.
// Revisit: rename things appropriately
bool HubCollaborationCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	// Revisit: define config data up front
	switch (ctrlId)
	{
	case IDC_HUB:
		if (_data.GetNewTopology ().HasHub ())
		{
			TheOutput.Display ("Warning! Switching from a satellite to a hub configuration is not automated.\n"
				"The distribution of scripts to existing satellites will initially result in\n"
				"the Dispatcher asking you questions to determine which project members \n"
				"are on which satellites, and what the names of the shares on those satellites are.");
		}
		if (_data.GetNewTopology ().UsesEmail ())
			_data.GetNewConfig ().MakeHubWithEmail ();
		else
			_data.GetNewConfig ().MakeHubNoEmail ();
		Enable (_hub.IsChecked ());
		break;
	case IDC_EMAIL_USAGE:
		if (_emailUsage.IsChecked ())
		{
			_emailProperties.Enable ();
			_data.GetNewConfig ().MakeHubWithEmail ();
			if (!_data.GetNewConfig ().GetInterClusterTransportToMe ().IsEmail ())
			{
				Transport transport (_data.GetNewConfig ().GetHubId ());
				if (!transport.IsEmail ())
					transport.Set ("", Transport::Unknown);
				// clear previous transport
				_data.GetNewConfig ().SetInterClusterTransportToMe (transport);
			}
		}
		else
		{
			_emailProperties.Disable ();
			_data.GetNewConfig ().MakeHubNoEmail ();
		}
		Enable (true);
		break;
	case IDC_EMAIL_PROPERTIES:
		{
			// Special code to handle the case of hub id different from hub remote transport.
			// Starting from Rebecca, users cannot define hub remote transports.
			std::string currentHubId (_data.GetNewConfig ().GetHubId ());
			Transport hubIdAsTransport (currentHubId);
			Transport const & hubRemoteTransport = _data.GetNewConfig ().GetInterClusterTransportToMe ();
			if (hubIdAsTransport != hubRemoteTransport)
			{
				std::string newHubId;
				if (hubRemoteTransport.IsEmail ())
					newHubId = hubRemoteTransport.GetRoute ();
				else if (hubIdAsTransport.IsEmail ())
					newHubId = currentHubId;
				HubIdAndRemoteTransportCtrl ctrl (
					currentHubId, 
					hubRemoteTransport.GetRoute (),
					newHubId);
				if (ThePrompter.GetData (ctrl))
				{
					_data.SetHubProperties (newHubId, Transport (newHubId));
					_data.AcceptChangesTo (ConfigData::bitHubId);
					_data.AcceptChangesTo (ConfigData::bitInterClusterTransportToMe);
					Enable (true);
				}
				break;
			}

			Email::Manager & emailMan = _data.GetEmailMan ();
			if (!emailMan.IsInTransaction ())
				emailMan.BeginEdit ();
			if (Email::RunOptionsSheet (_email.GetString (), 
										emailMan,
										GetWindow (), 
										_data.GetMsgPrepro ()))
			{
				_data.GetNewConfig ().SetUseEmail (true);
				Enable (true);
			}
		}
		break;
	case IDC_MY_NETWORK_PROPERTIES:
		{
			Assert (_hub.IsChecked ());
			Transport transport (_data.GetNewConfig ().GetActiveIntraClusterTransportToMe ());
			if (transport.IsEmail ()) // special case of mobile config
				transport = _data.GetNewConfig ().GetIntraClusterTransportToMe (Transport::Network);
			MyNetworkProperties networkProp (
									_data.GetNewConfig ().GetPublicInboxPath (),
									transport);
			MyNetworkPropertiesCtrl ctrl (networkProp);
			if (ThePrompter.GetData (ctrl))
			{
				_data.SetIntraClusterTransportToMe (networkProp.GetIntraClusterTransportToMe ());
				_data.AcceptChangesTo (ConfigData::bitActiveIntraClusterTransportToMe);
				Enable (true);
			}
		}
		break;
	}
	return false;
}

void HubCollaborationCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData & cfg = _data.GetNewConfig ();

	if (!cfg.GetTopology ().IsHub ())
		return;

	if (!_data.IsHubAdvanced ())
	{
		// old hub id == old transport => new hub id == newTransport
		std::string newHubId = _email.GetString ();
		if (_emailUsage.IsChecked ())
		{
			Transport transport (newHubId);
			if (transport.IsEmail ())
			{
				cfg.SetHubId (newHubId);
				cfg.SetInterClusterTransportToMe (transport);
			}
			else
			{
				TheOutput.Display ("Please specify an e-mail address for your hub.");
				result = -1;
			}
		}
		else
		{
			if (newHubId.empty () || IsNocaseEqual ("Unknown", newHubId))
			{
				TheOutput.Display ("Please specify a valid hub id.");
				result = -1;
			}
			else
				cfg.SetHubId (newHubId);
		}
	}
	// Revisit: if switch from no email to email and not tested, 
	// "Use E-mail Options button to configure and test your e-mail connection"
	if (!_data.GetEmailMan ().IsValid ()) // use tmp email manager
	{
		if (result == 0) //  no error was displayed before
			DisplayEmailCfgErr ();

		result = -1;
	}
	// The network
	Transport satTransport (_hubShare.GetString ());
	if (satTransport.IsUnknown ())
	{
		if (result == 0) //  no error was displayed before
			TheOutput.Display ("Please specify a valid share name for your Public Inbox");
	
		result = -1;
	}
	else
		cfg.SetActiveIntraClusterTransportToMe (satTransport);


	if (result == 0)
		_data.AcceptChangesTo (_modified);
}

void HubCollaborationCtrl::Enable (bool enable)
{
	ConfigData const config = _data.GetNewConfig ();
	// If we are "really" using email, display email address, otherwise use hub id
	if (config.GetTopology ().UsesEmail ())
	{
		if (config.GetInterClusterTransportToMe ().IsEmail ())
		{
			_email.SetText (config.GetInterClusterTransportToMe ().GetRoute ().c_str ());
			_emailUsage.Check ();
		}
		else if (config.GetInterClusterTransportToMe ().IsUnknown ())
		{
			// Let them initialize email
			_email.SetText (config.GetHubId ().c_str ());
			_emailUsage.Check ();
		}
		else
		{
			_email.SetText (config.GetHubId ().c_str ());
			_emailUsage.UnCheck ();
		}
	}
	else
	{
		_email.SetText (config.GetHubId ().c_str ());
		_emailUsage.UnCheck ();
	}

	Transport hubTransport (config.GetActiveIntraClusterTransportToMe ());
	if (hubTransport.IsEmail ()) // special case of mobile config
		hubTransport = config.GetIntraClusterTransportToMe (Transport::Network);
	_hubShare.SetText (hubTransport.GetRoute ().c_str ());

	if (enable)
	{
		_hub.Check ();
		// if hub id != transport, force the use of Advanced button
		if (_data.IsHubAdvanced ())
			_email.Disable ();
		else
			_email.Enable ();
		_emailUsage.Enable ();
		_emailProperties.Enable ();
		_myNetworkProperties.Enable ();
	}
	else
	{
		_hub.UnCheck ();
		_email.Disable ();
		_emailUsage.Disable ();
		_emailProperties.Disable ();
		_myNetworkProperties.Disable ();
	}
}

//------------------
// Network Satellite
//------------------
bool SatCollaborationCtrl::OnInitDialog () throw (Win::Exception)
{
	_satellite.Init (GetWindow (), IDC_SATELLITE);
	_hubEmail.Init (GetWindow (), IDC_HUB_EMAIL);
	_hubShare.Init (GetWindow (), IDC_HUB_SHARE);
	_satShare.Init (GetWindow (), IDC_SAT_SHARE);
	_hubNetworkProperties.Init (GetWindow (), IDC_NETWORK_PROPERTIES);
	_satNetworkProperties.Init (GetWindow (), IDC_MY_NETWORK_PROPERTIES);

	_modified.Set (ConfigData::bitTopologyCfg);
	_modified.Set (ConfigData::bitHubId);
	_modified.Set (ConfigData::bitInterClusterTransportToMe);
	_modified.Set (ConfigData::bitActiveIntraClusterTransportToMe);
	_modified.Set (ConfigData::bitActiveTransportToHub);

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SHARE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SHARE2_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

void SatCollaborationCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	if (_data.GetNewTopology ().IsSatellite ())
		Enable (true);
	else
		Enable (false);
}

bool SatCollaborationCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_SATELLITE:
		_data.GetNewConfig ().MakeSatellite ();
		Enable (_satellite.IsChecked ());
		break;
	case IDC_NETWORK_PROPERTIES:
		{
			Assert (_satellite.IsChecked ());
			ConfigData const & config = _data.GetNewConfig ();
			Transport transport (config.GetActiveTransportToHub ());
			if (transport.IsEmail ()) // special case of mobile config
				transport = config.GetTransportToHub (Transport::Network);
			HubNetworkProperties networkProp (transport);
			HubNetworkPropertiesCtrl ctrl (networkProp);
			if (ThePrompter.GetData (ctrl))
			{
				_data.SetTransportToHub (networkProp.GetTransportToHub ());
				_data.AcceptChangesTo (ConfigData::bitActiveTransportToHub);
				Enable (true);
			}
		}
		break;
	case IDC_MY_NETWORK_PROPERTIES:
		{
			Assert (_satellite.IsChecked ());
			ConfigData const & config = _data.GetNewConfig ();
			Transport transport (config.GetActiveIntraClusterTransportToMe ());
			if (transport.IsEmail ()) // special case of mobile config
				transport = config.GetIntraClusterTransportToMe (Transport::Network);
			MyNetworkProperties networkProp (
									config.GetPublicInboxPath (),
									transport);
			MyNetworkPropertiesCtrl ctrl (networkProp);
			if (ThePrompter.GetData (ctrl))
			{
				_data.SetIntraClusterTransportToMe (networkProp.GetIntraClusterTransportToMe ());
				_data.AcceptChangesTo (ConfigData::bitActiveIntraClusterTransportToMe);
				Enable (true);
			}
		}
		break;
	}
	return false;
}

void SatCollaborationCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData & cfg = _data.GetNewConfig ();

	if (!cfg.GetTopology ().IsSatellite ())
		return;

	if (!_data.IsHubAdvanced ())
	{
		std::string newHubId = _hubEmail.GetString ();
		if (newHubId.empty () || IsNocaseEqual (newHubId, "Unknown"))
		{
			TheOutput.Display ("Please specify a valid hub name/email address or use Advanced settings.");
			result = -1;
		}
		else
		{
			cfg.SetHubId (newHubId);
			Transport hubRemTransport (newHubId);
			if (hubRemTransport.IsEmail ())
			{
				cfg.SetInterClusterTransportToMe (hubRemTransport);
			}
		}
	}

	Transport hubTransport (_hubShare.GetString ());
	if (!hubTransport.IsNetwork ())
	{
		if (result == 0) //  no error was displayed before
			TheOutput.Display ("Please specify a valid hub network share");
	
		result = -1;
	}
	else
		cfg.SetActiveTransportToHub (hubTransport);

	Transport myTransport (_satShare.GetString ());
	if (!myTransport.IsNetwork ())
	{
		if (result == 0) //  no error was displayed before
			TheOutput.Display ("Please specify a valid network share");
	
		result = -1;
	}
	else
		cfg.SetActiveIntraClusterTransportToMe (myTransport);

	if (result == 0)
		_data.AcceptChangesTo (_modified);
}

void SatCollaborationCtrl::Enable (bool enable)
{
	ConfigData & cfg = _data.GetNewConfig ();
	if (_hubEmail.GetTextLength () == 0)
		_hubEmail.SetText (cfg.GetHubId ().c_str ());

	Transport hubTransport (cfg.GetTransportToHub (Transport::Network));
	_hubShare.SetText (hubTransport.GetRoute ().c_str ());
	Transport const & myTransport = cfg.GetActiveIntraClusterTransportToMe ();
	if (myTransport.IsEmail ()) // off-site configurations
		_satShare.SetText (cfg.GetIntraClusterTransportToMe (Transport::Network).GetRoute ().c_str ());
	else
		_satShare.SetText (myTransport.GetRoute ().c_str ());

	if (enable)
	{
		_satellite.Check ();
		// if hub id != transport, force the use of Advanced button
		if (_data.IsHubAdvanced ())
			_hubEmail.Disable ();
		else
			_hubEmail.Enable ();
		_hubShare.Enable ();
		_hubNetworkProperties.Enable ();
		_satNetworkProperties.Enable ();
	}
	else
	{
		_satellite.UnCheck ();
		_hubEmail.Disable ();
		_hubShare.Disable ();
		_hubNetworkProperties.Disable ();
		_satNetworkProperties.Disable ();
	}
}

//-------------
// Off-Site Hub
//-------------

bool TemporaryHubCollaborationCtrl::OnInitDialog () throw (Win::Exception)
{
	_tempHub.Init (GetWindow (), IDC_TEMP_HUB);
	_email.Init (GetWindow (), IDC_HUB_EMAIL);
	_myEmail.Init (GetWindow (), IDC_MY_EMAIL);
	_emailProperties.Init (GetWindow (), IDC_EMAIL_PROPERTIES);
	_hubShare.Init (GetWindow (), IDC_HUB_SHARE);

	_modified.Set (ConfigData::bitTopologyCfg);
	_modified.Set (ConfigData::bitActiveTransportToHub);

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

void TemporaryHubCollaborationCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	Enable (_data.GetNewConfig ().GetTopology ().IsTemporaryHub ());
}

bool TemporaryHubCollaborationCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_TEMP_HUB:
		{
			if (!_tempHub.IsChecked ())
				break;
			if (!_data.GetNewTopology ().IsSatellite () && !_data.GetNewTopology ().IsTemporaryHub ())
			{
				TheOutput.Display ("You have to be a satellite first if you want to become an off-site hub.");
				_tempHub.UnCheck ();
				break;
			}
			Transport myTransport (_myEmail.GetString ());
			if (!myTransport.IsEmail ())
			{
				TheOutput.Display ("You have to enter the hub's email address first "
								   "in order to become an off-site hub.\nGo back to"
								   " satellite settings and click the Advanced button.");
				_tempHub.UnCheck ();
				break;
			}
			else
			{
				_data.GetNewConfig ().MakeTemporaryHub ();
				Enable (_tempHub.IsChecked ());
			}
		}
		break;
	case IDC_EMAIL_PROPERTIES:
		{
			Email::Manager & emailMan = _data.GetEmailMan ();
			if (!emailMan.IsInTransaction ())
				emailMan.BeginEdit ();
			if (Email::RunOptionsSheet (_myEmail.GetString (), 
										emailMan, 
										GetWindow (), 
										_data.GetMsgPrepro ()))
			{
				Enable (true);
			}
		}
		break;
	}
	return false;
}

void TemporaryHubCollaborationCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;
	if (!_data.GetNewTopology ().IsTemporaryHub ())
		return;

	Transport hubShare (_hubShare.GetString ());
	_data.GetNewConfig ().SetActiveTransportToHub (hubShare);
	if (!_data.GetEmailMan ().IsValid ())
	{
		DisplayEmailCfgErr ();
		result = -1;
	}

	if (result == 0)
		_data.AcceptChangesTo (_modified);
}

void TemporaryHubCollaborationCtrl::Enable (bool enable)
{
	ConfigData const & cfg = _data.GetNewConfig ();

	_email.SetText (cfg.GetHubId ().c_str ());
	_myEmail.SetText (cfg.GetInterClusterTransportToMe ().GetRoute ().c_str ());

	Transport hubTransport (cfg.GetActiveTransportToHub ());
	if (hubTransport.IsEmail ()) // special case of mobile config
		hubTransport = cfg.GetTransportToHub (Transport::Network);
	_hubShare.SetText (hubTransport.GetRoute ().c_str ());

	if (enable)
	{
		_tempHub.Check ();
		_emailProperties.Enable ();
	}
	else
	{
		_tempHub.UnCheck ();
		_emailProperties.Disable ();
	}
}

//-------------------
// Off-Site Satellite
//-------------------
bool RemoteSatCollaborationCtrl::OnInitDialog () throw (Win::Exception)
{
	_remoteSat.Init (GetWindow (), IDC_REMOTE_SATELLITE);
	_hubEmail.Init (GetWindow (), IDC_HUB_EMAIL);
	_myEmail.Init (GetWindow (), IDC_MY_EMAIL);
	_emailProperties.Init (GetWindow (), IDC_EMAIL_PROPERTIES);

	_modified.Set (ConfigData::bitTopologyCfg);
	_modified.Set (ConfigData::bitActiveIntraClusterTransportToMe);
	_modified.Set (ConfigData::bitActiveTransportToHub);

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

void RemoteSatCollaborationCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;
	ConfigData const & cfg = _data.GetNewConfig ();

	_hubEmail.SetText (cfg.GetInterClusterTransportToMe ().GetRoute ().c_str ());
	_myEmail.SetText (cfg.GetIntraClusterTransportToMe (Transport::Email).GetRoute ().c_str ());

	Enable (cfg.GetTopology ().IsRemoteSatellite ());
}

bool RemoteSatCollaborationCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_REMOTE_SATELLITE:
		{
			if (!_remoteSat.IsChecked ())
				break;
			if (!_data.GetNewTopology ().IsSatellite () && !_data.GetNewTopology ().IsRemoteSatellite ())
			{
				TheOutput.Display ("You have to be a satellite first if you want to become an off-site satellite.");
				_remoteSat.UnCheck ();
				break;
			}
			Transport hubTransport (_hubEmail.GetString ());
			if (!hubTransport.IsEmail ())
			{
				TheOutput.Display ("You have to define the hub's email address first "
								   "in order to become an off-site satellite.\nGo back"
								   " to satellite settings and click the Advanced button.");
				_remoteSat.UnCheck ();
				break;
			}
			else
			{
				_data.GetNewConfig ().MakeRemoteSatellite ();
				Enable (_remoteSat.IsChecked ());
			}
		}
		break;
	case IDC_EMAIL_PROPERTIES:
		{
			Email::Manager & emailMan = _data.GetEmailMan ();
			if (!emailMan.IsInTransaction ())
				emailMan.BeginEdit ();
			Email::RunOptionsSheet (_myEmail.GetString (), 
									emailMan, 
									GetWindow (), 
									_data.GetMsgPrepro ());
		}
		break;
	}
	return false;
}

void RemoteSatCollaborationCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;
	if (!_data.GetNewTopology ().IsRemoteSatellite ())
		return;

	Transport myTransport (_myEmail.GetString ());
	Transport hubEmail (_data.GetNewConfig ().GetInterClusterTransportToMe ());
	if (!myTransport.IsEmail ())
	{
		TheOutput.Display ("Please specify a valid email address.");
		result = -1;
	}
	else if (IsNocaseEqual (myTransport.GetRoute (), hubEmail.GetRoute ()))
	{
		TheOutput.Display ("Your email address must be different from your hub's email address.");
		result = -1;
	}
	else if (!_data.GetEmailMan ().IsValid ())
	{
		DisplayEmailCfgErr ();
		result = -1;
	}
	_data.GetNewConfig ().SetActiveIntraClusterTransportToMe (myTransport);
	_data.GetNewConfig ().SetActiveTransportToHub (hubEmail);

	if (result == 0)
		_data.AcceptChangesTo (_modified);
}

void RemoteSatCollaborationCtrl::Enable (bool enable)
{
	if (enable)
	{
		_remoteSat.Check ();
		_myEmail.Enable ();
		_emailProperties.Enable ();
	}
	else
	{
		_remoteSat.UnCheck ();
		_myEmail.Disable ();
		_emailProperties.Disable ();
	}
}

bool HubIdAndRemoteTransportCtrl::OnInitDialog () throw (Win::Exception)
{
	_currentHubIdEdit.Init (GetWindow (), IDC_HUB_ID);
	_hubRemoteTransportEdit.Init (GetWindow (), IDC_HUB_REMOTE_TRANS);
	_newHubIdEdit.Init (GetWindow (), IDC_HUB_ID_NEW);

	Assert (!_currentHubId.empty ());
	_currentHubIdEdit.SetText (_currentHubId.c_str ());
	if (!_hubRemoteTransport.empty ())
		_hubRemoteTransportEdit.SetText (_hubRemoteTransport.c_str ());
	if (!_newHubId.empty ())
		_newHubIdEdit.SetText (_newHubId.c_str ());

	return true;
}

bool HubIdAndRemoteTransportCtrl::OnApply () throw ()
{
	_newHubId = _newHubIdEdit.GetTrimmedString ();
	if (Email::IsValidAddress (_newHubId))
	{
		EndOk ();
	}
	else
	{
		TheOutput.Display ("Please specify an e-mail address for your hub.");
	}
	return true;
}
