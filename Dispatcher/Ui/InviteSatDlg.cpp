// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "InviteSatDlg.h"
#include "BrowseForFolder.h"
#include "OutputSink.h"
#include "Prompter.h"

#include <File/Path.h>
#include <StringOp.h>
#include <Mail/EmailAddress.h>

bool InviteSatData::IsValid () const
{
	if (_isRemove)
		return true;

	if (_path.empty ())
		return false;

	if (HasOffsiteSats ())
		return Email::IsValidAddress (_path) || IsValidUNC ();
	else
		return IsValidUNC ();
}

bool InviteSatData::IsValidUNC () const
{
	if (!FilePath::IsValid (_path))
		return false;

	FilePathSeq seq (_path.c_str ());
	if (!seq.IsUNC ())
		return false;

	return true;
}

void InviteSatData::DisplayError (Win::Dow::Handle owner)
{
	Assert (!IsValid ());
	if (_path.empty ())
	{
		TheOutput.Display ("Please specify a network path to the invited user's computer.", 
							Out::Information, 
							owner);
	}
	else if (!HasOffsiteSats ())
	{
		if (!FilePath::IsValid (_path))
		{
			TheOutput.Display ("The forwarding path contains some illegal characters.\n"
				"Please specify a valid path.",
				Out::Information, 
				owner);
		}
		else
		{
			TheOutput.Display ("Please specify a network path to the invited user's computer."
				"\n(usually \\\\COMPUTER-NAME\\CODECOOP)", 
				Out::Information, 
				owner);
		}
	}
	else
	{
		TheOutput.Display ("Please specify a valid network path or a valid e-mail address"
			"\nof the invited user's computer.",
			Out::Information, 
			owner);
	}
}

bool InviteSatCtrl::OnInitDialog () throw (Win::Exception)
{
	_userName.Init (GetWindow (), IDC_USER_NAME);
	_project.Init (GetWindow (), IDC_PROJECT_NAME);
	_computerName.Init (GetWindow (), IDC_COMPUTER_NAME);
	_isOnSat.Init (GetWindow (), IDC_SAT_EXISTS);
	_isRemove.Init (GetWindow (), IDC_REMOVE_SCRIPT);
	_path.Init (GetWindow (), IDC_PATH);
	_browse.Init (GetWindow (), IDC_BROWSE);

	_userName.SetText (_data.GetUserName ());
	_project.SetText (_data.GetProject ());
	_computerName.SetText (_data.GetComputer ());
	_isOnSat.Check ();

	std::string defaultPath ("\\\\");
	defaultPath += _data.GetComputer ();
	defaultPath += "\\CODECOOP";
	_path.SetString (defaultPath);

	if (_data.HasOffsiteSats ())
		_browse.SetText ("Advanced...");
	else
		_browse.SetText ("Browse...");

	return true;
}

bool InviteSatCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_BROWSE:
		if (_data.HasOffsiteSats ())
		{
			std::string newTransport = _path.GetTrimmedString ();
			SatTransportCtrl ctrl (_data.GetComputer (), newTransport, _data.GetOffsiteSats ());
			if (ThePrompter.GetData (ctrl))
			{
				_path.SetString (newTransport);
			}
		}
		else
		{
			std::string folder;
			if (BrowseForSatelliteShare (folder, GetWindow ()))
				_path.SetString (folder);
		}
		return true;
	case IDC_SAT_EXISTS:
		_isOnSat.Check ();
		_isRemove.UnCheck ();
		_browse.Enable ();
		_path.Enable ();
	case IDC_REMOVE_SCRIPT:
		_isRemove.Check ();
		_isOnSat.UnCheck ();
		_path.Disable ();
		_browse.Disable ();
	};
	return false;
}

bool InviteSatCtrl::OnApply () throw ()
{
	bool const isRemove = _isRemove.IsChecked ();
	_data.SetIsRemove (isRemove);
	if (!isRemove)
		_data.SetPath (_path.GetTrimmedString ());

	if (_data.IsValid ())
		EndOk ();
	else
		_data.DisplayError (GetWindow ());

	return true;
}

//====================================
bool SatTransportCtrl::OnInitDialog () throw (Win::Exception)
{
	_info.Init (GetWindow (), IDC_INFO);
	_isNetSat.Init (GetWindow (), IDC_NET_SAT);
	_isOffSat.Init (GetWindow (), IDC_OFF_SAT);
	_path.Init (GetWindow (), IDC_PATH);
	_browse.Init (GetWindow (), IDC_BROWSE);
	_offsites.Init (GetWindow (), IDC_OFFSITE_SAT_EMAILS);

	std::string info ("Specify transport to ");
	info += _computerName;
	info += " satellite.";
	_info.SetText (info);

	for (NocaseSet::const_iterator off = _offsiteSats.begin (); off != _offsiteSats.end (); ++off)
		_offsites.AddItem (off->c_str ());

	if (Email::IsValidAddress (_transport))
	{
		ActivateOffsiteControls ();
		int sel = _offsites.FindItemByName (_transport.c_str ());
		if (sel >= 0)
			_offsites.Select (sel);
	}
	else
	{
		ActivateNetControls ();
		_path.SetString (_transport);
	}
	
	return true;
}

bool SatTransportCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_BROWSE:
		{
			std::string folder;
			if (BrowseForSatelliteShare (folder, GetWindow ()))
				_path.SetString (folder);
		}
		return true;
	case IDC_NET_SAT:
		ActivateNetControls ();
		break;
	case IDC_OFF_SAT:
		ActivateOffsiteControls ();
		break;
	};
	return false;
}

bool SatTransportCtrl::OnApply () throw ()
{
	if (_isNetSat.IsChecked ())
	{
		_transport = _path.GetTrimmedString ();
		// only the simplest validation here
		if (_transport.empty ())
		{
			TheOutput.Display ("Please specify the network path.");
			return true;
		}
	}
	else
	{
		if (_offsites.HasSelection ())
		{
			_offsites.GetItemString (_offsites.Selection (), _transport);
		}
		else
		{
			TheOutput.Display ("Please select one of the e-mail addresses.");
			return true;
		}
	}
	EndOk ();
	return true;
}

void SatTransportCtrl::ActivateNetControls ()
{
	_isNetSat.Check ();
	_isOffSat.UnCheck ();
	_path.Enable ();
	_browse.Enable ();
	_offsites.Disable ();
}

void SatTransportCtrl::ActivateOffsiteControls ()
{
	_isNetSat.UnCheck ();
	_isOffSat.Check ();
	_path.Disable ();
	_browse.Disable ();
	_offsites.Enable ();
}
