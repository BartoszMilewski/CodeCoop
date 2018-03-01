// ---------------------------------
// (c) Reliable Software 1999 - 2008
// ---------------------------------

#include "precompiled.h"
#include "HubPathPage.h"
#include "OutputSink.h"
#include "BrowseForFolder.h"

#include <Net/NetShare.h>

bool SatelliteTransportsHandler::OnInitDialog () throw (Win::Exception)
{
	_pathEdit.Init (GetWindow (), IDC_PATH);
	_pathBrowse.Init (GetWindow (), IDC_BROWSE);
	_hubIdEdit.Init (GetWindow (), IDC_HUB_ID);
    
	_modified.Set (ConfigData::bitActiveTransportToHub);
	_modified.Set (ConfigData::bitHubId);
	// We have to create a PublicInbox share
	_modified.Set (ConfigData::bitInterClusterTransportToMe);

	ConfigData & newConfig = _wizardData.GetNewConfig ();

	std::string myPublicInboxShareName = newConfig.GetActiveIntraClusterTransportToMe ().GetRoute ();
	if (myPublicInboxShareName.empty ())
	{
		Net::LocalPath sharePath;
		sharePath.DirDown ("CODECOOP");
		myPublicInboxShareName = sharePath.ToString ();
		Transport transportToMe (myPublicInboxShareName);
		newConfig.SetActiveIntraClusterTransportToMe (transportToMe);
	}

	_pathEdit.SetString (newConfig.GetActiveTransportToHub ().GetRoute ().c_str ());
	if (!IsNocaseEqual (newConfig.GetHubId (), "Unknown"))
		_hubIdEdit.SetString (newConfig.GetHubId ().c_str ());

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SHARE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

bool SatelliteTransportsHandler::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    if (ctrlId == IDC_BROWSE)
    {
		std::string path;
		if (BrowseForHubShare (path, GetWindow ()))
		{
            _pathEdit.SetString (path);
		}
        return true;
    };
    return false;
}

void SatelliteTransportsHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	if (acceptPage)
	{
		ConfigData & newConfig = _wizardData.GetNewConfig ();
		newConfig.SetActiveTransportToHub (Transport (_pathEdit.GetString ()));
		newConfig.SetHubId (_hubIdEdit.GetTrimmedString ());
	}
}

bool SatelliteTransportsHandler::Validate () const throw (Win::Exception)
{
	if (!TheTransportValidator->ValidateExcludePublicInbox (
				_wizardData.GetNewConfig ().GetActiveTransportToHub (), 
				"You cannot specify your own Public Inbox\n"
				"as the forwarding path to your hub.",
				GetWindow ()))
	{
		return false;
	}

	if (_wizardData.GetNewConfig ().GetHubId ().empty ())
	{
		TheOutput.Display ("Please enter the hub's email address\n"
			"(or use the Dispatcher's Collaboration Settings for advanced configuration)");
		return false;
	}
	return true;
}

bool HubTransportsHandler::OnInitDialog () throw (Win::Exception)
{
	_hubIdEdit.Init (GetWindow (), IDC_HUB_ID);
	_hubShare.Init (GetWindow (), IDC_HUB_SHARE);
    
	_modified.Set (ConfigData::bitHubId);
	// hub remote transport may also be changed 
	// (a case when the original hub id is either empty or "Unknown")
	_modified.Set (ConfigData::bitInterClusterTransportToMe);

	ConfigData & newConfig = _wizardData.GetNewConfig ();
	std::string myPublicInboxShareName = newConfig.GetActiveIntraClusterTransportToMe ().GetRoute ();
	if (myPublicInboxShareName.empty ())
	{
		Net::LocalPath sharePath;
		sharePath.DirDown ("CODECOOP");
		myPublicInboxShareName = sharePath.ToString ();
		Transport transportToMe (myPublicInboxShareName);
		newConfig.SetActiveIntraClusterTransportToMe (transportToMe);
	}
	_hubShare.SetText (myPublicInboxShareName.c_str ());
	if (!IsNocaseEqual (newConfig.GetHubId (), "Unknown"))
		_hubIdEdit.SetString (newConfig.GetHubId ().c_str ());

	Win::StaticImage stc;
	stc.Init (GetWindow (), IDC_HUB_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SAT_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));
	stc.Init (GetWindow (), IDC_SHARE_IMAGE);
	stc.ReplaceBitmapPixels (Win::Color (0xff, 0xff, 0xff));

	return true;
}

void HubTransportsHandler::RetrieveData (bool acceptPage) throw (Win::Exception)
{
	if (acceptPage)
	{
		ConfigData & newConfig = _wizardData.GetNewConfig ();
		newConfig.SetHubId (_hubIdEdit.GetTrimmedString ());
	}
}

bool HubTransportsHandler::Validate (Win::Dow::Handle win) const throw (Win::Exception)
{
	if (_wizardData.GetNewConfig ().GetHubId ().empty ())
	{
		TheOutput.Display ("Please enter the hub's email for identification purposes\n"
			"(You may have to invent a unique email address)");
		return false;
	}
	return true;
}



