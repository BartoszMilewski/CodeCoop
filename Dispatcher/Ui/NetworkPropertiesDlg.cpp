// ----------------------------------
// (c) Reliable Software, 2002 - 2006
// ----------------------------------

#include "precompiled.h"
#include "NetworkPropertiesDlg.h"
#include "ConfigData.h"
#include "BrowseForFolder.h"
#include "OutputSink.h"
#include "resource.h"

#include <Net/NetShare.h>
#include <LightString.h>

bool ValidateShare (Win::Dow::Handle win, Transport const & transport, bool thisMachine)
{
	if (transport.GetRoute ().empty () || transport.IsUnknown ())
	{
		TheOutput.Display ("Please specify a valid Public Inbox share.",
							Out::Information, win);
		return false;
	}
	if (transport.IsNetwork ())
	{
		FilePath piShare (transport.GetRoute ());
		PathSplitter splitShare (piShare.GetDir ());
		if (!FilePath::IsValid (piShare.GetDir ()))
		{
			TheOutput.Display ("Public Inbox share contains some illegal characters.",
							   Out::Information, win);
			return false;
		}
		
		if (!splitShare.IsValidShare ())
		{
			TheOutput.Display ("Public Inbox share must be of form:\n\\\\machine\\share", 
							   Out::Information, win);
			return false;
		}
		
		if (thisMachine)
		{
			Net::LocalPath sharePath;
			if (!piShare.HasPrefix (sharePath))
			{
				Msg msg;
				msg << "Public Inbox share must start with this machine name which is "
					<< sharePath.GetDir ();
				TheOutput.Display (msg.c_str (), Out::Information, win);
				return false;
			}
		}
		else
		{
			if (!TheTransportValidator->ValidateExcludePublicInbox (
									transport,
									"You cannot specify your own Public Inbox\n"
									"as the forwarding path to your hub.",
									win))
			return false;
		}
	}
	return true;
}

MyNetworkPropertiesCtrl::MyNetworkPropertiesCtrl (MyNetworkProperties & data)
	: Dialog::ControlHandler (IDD_MY_NETWORK_PROPERTIES),
	  _data (data)
{}

bool MyNetworkPropertiesCtrl::OnInitDialog () throw (Win::Exception)
{
	_piPath.Init (GetWindow (), IDC_PREFS_PI_PATH);
	_share.Init (GetWindow (), IDC_PREFS_SHARE_EDIT);

	FilePath piPath (_data.GetPublicInboxPath ().GetDir ());
	_piPath.SetString (piPath.GetDir ());
	_share.SetString (_data.GetIntraClusterTransportToMe ().GetRoute ().c_str ());

	return true;
}

bool MyNetworkPropertiesCtrl::OnApply () throw ()
{
	Transport transport (_share.GetString ());
	if (ValidateShare (GetWindow (), transport, true))
	{
		_data.SetIntraClusterTransportToMe (transport);
		EndOk ();
	}
	return true;
}

HubNetworkPropertiesCtrl::HubNetworkPropertiesCtrl (HubNetworkProperties & data)
	: Dialog::ControlHandler (IDD_NETWORK_PROPERTIES),
	  _data (data)
{}

bool HubNetworkPropertiesCtrl::OnInitDialog () throw (Win::Exception)
{
	_hubTransport.Init (GetWindow (), IDC_PREFS_FWD_EDIT);
	_browse.Init (GetWindow (), IDC_PREFS_FWD_BROWSE);

	_hubTransport.SetString (_data.GetTransportToHub ().GetRoute ().c_str ());

	return true;
}

bool HubNetworkPropertiesCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_PREFS_FWD_BROWSE)
	{
		std::string path;
		if (BrowseForHubShare (path, GetWindow ()))
		{
			_hubTransport.SetString (path);
		}
		return true;
	}
	return false;
}

bool HubNetworkPropertiesCtrl::OnApply () throw ()
{
	Transport transport (_hubTransport.GetString ());
	if (ValidateShare (GetWindow (), transport, false))
	{
		_data.SetTransportToHub (transport);
		EndOk ();
	}
	return true;
}
