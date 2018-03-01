//------------------------------------
//  (c) Reliable Software, 2000 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProjectPropsDlg.h"
#include "ProjectOptionsEx.h"
#include "ProjectDb.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "DistributorInfo.h"
#include "BrowseForFolder.h"

#include <Ctrl/Output.h>
#include <Win/Dialog.h>
#include <Com/Shell.h>
#include <Sys/WinString.h>

bool ProjectOptionsCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_autoSynch.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_SYNCH);
	_autoJoin.Init (dlgWin, IDC_PROJ_OPTIONS_AUTO_JOIN);
	_keepCheckedOut.Init (dlgWin, IDC_PROJ_OPTIONS_KEEP_CHECKED_OUT);
	_checkoutNotification.Init (dlgWin, IDC_START_CHECKOUT_NOTIFICATIONS);
	_autoInvite.Init (dlgWin, IDC_OPTIONS_AUTO_INVITE);
	_projectPath.Init (dlgWin, IDC_OPTIONS_PATH);
	_pathBrowse.Init (dlgWin, IDC_OPTIONS_BROWSE);

	InitializeControls ();
	return true;
}

bool ProjectOptionsCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_OPTIONS_AUTO_INVITE && Win::SimpleControl::IsClicked (notifyCode))
	{
		if (_autoInvite.IsChecked ())
		{
			_projectPath.Enable ();
			_pathBrowse.Enable ();
		}
		else
		{
			_projectPath.Disable ();
			_pathBrowse.Disable ();
		}
	}
	else if (ctrlId == IDC_OPTIONS_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
	{
		std::string path = _projectPath.GetString ();
		if (BrowseForAnyFolder (path,
								GetWindow (),
								"Select folder (existing or not) where new projects will be created.",
								path.c_str ()))
		{
			_projectPath.SetString (path);
		}
	}
    return true;
}

void ProjectOptionsCtrl::OnSetActive (long & result) throw (Win::Exception)
{
	result = 0;		// Assume everything is OK
	InitializeControls ();
}

void ProjectOptionsCtrl::OnCancel (long & result) throw (Win::Exception)
{
	_options.Clear ();
	_options.SetAutoInviteProjectPath ("");
	_options.SetAutoInvite (false);
}

void ProjectOptionsCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;		// Assume everything is OK
	_options.SetAutoSynch (_autoSynch.IsChecked ());
	if (_options.IsProjectAdmin () && !_options.IsDistribution ())
		_options.SetAutoJoin (_autoJoin.IsChecked ());
	_options.SetKeepCheckedOut (_keepCheckedOut.IsChecked ());
	_options.SetCheckoutNotification (_checkoutNotification.IsChecked ());
	if (_options.IsProjectAdmin () &&
		_options.IsAutoSynch () &&
		!_options.IsAutoJoin ())
	{
		Out::Answer userChoice = TheOutput.Prompt (
			"You are the Admin for this project and you have selected to\n"
			"automatically execute all incoming synchronization changes,\n"
			"but not to automatically accept join requests.\n\n"
			"You'll have to occasionally check for join request and execute them manually\n\n"
			"Do you want to continue with your current settings (automatic join request\n"
			"processing not selected)?",
			Out::PromptStyle (Out::YesNo, Out::No));
		if (userChoice == Out::No)
			result = 1;	// Don't close dialog
	}

	_options.SetAutoInvite (_autoInvite.IsChecked ());
	_options.SetAutoInviteProjectPath (_projectPath.GetString ());
	if (!_options.ValidateAutoInvite (GetWindow ()))
		result = 1;	// Don't close dialog
}

void ProjectOptionsCtrl::InitializeControls ()
{
	if (_options.IsAutoSynch ())
		_autoSynch.Check ();
	else
		_autoSynch.UnCheck ();

	if (_options.IsProjectAdmin ())
	{
		if (_options.IsDistribution ())
		{
			_autoJoin.UnCheck ();
			_autoJoin.Disable ();
		}
		else
		{
			_autoJoin.Enable ();
			if (_options.IsAutoJoin ())
				_autoJoin.Check ();
			else
				_autoJoin.UnCheck ();
		}
	}
	else
	{
		_autoJoin.UnCheck ();
		_autoJoin.Disable ();
	}

	if (_options.IsReceiver ())
	{
		_keepCheckedOut.Disable ();
		_checkoutNotification.Disable ();
	}
	else
	{
		if (_options.IsKeepCheckedOut ())
			_keepCheckedOut.Check ();
		else
			_keepCheckedOut.UnCheck ();

		if (_options.IsCheckoutNotification ())
			_checkoutNotification.Check ();
		else
			_checkoutNotification.UnCheck ();
	}

	_projectPath.SetString (_options.GetAutoInviteProjectPath ());
	if (_options.IsAutoInvite ())
	{
		_autoInvite.Check ();
	}
	else
	{
		_autoInvite.UnCheck ();
		_projectPath.Disable ();
		_pathBrowse.Disable ();
	}
}

bool ProjectDistributorCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_distributor.Init (dlgWin, IDC_PROJ_OPTIONS_DISTRIBUTOR);
	_noBranching.Init (dlgWin, IDC_PROJ_OPTIONS_DISALLOW_BRANCHING);
	_frame.Init (dlgWin, IDC_DISTRIBUTOR_FRAME);
	_allBcc.Init (dlgWin, IDC_PROJ_OPTIONS_ALL_BCC_RECIPIENTS);
	_singleRecipient.Init (dlgWin, IDC_PROJ_OPTIONS_SINGLE_TO_RECIPIENT);
	_status.Init (dlgWin, IDC_DISTRIBUTOR_STATUS);
	_license.Init (dlgWin, IDC_DISTRIBUTOR_LICENSE);
    _buyLicense.Init (dlgWin, IDC_LICENSE_PURCHASE);

	if (_options.IsDistribution ())
	{
		_status.Hide ();
		_distributor.Check ();
		if (_options.IsNoBranching ())
			_noBranching.Check ();
		else
			_noBranching.UnCheck ();
		if (_options.UseBccRecipients ())
			_allBcc.Check ();
		else
			_singleRecipient.Check ();
		if (!_options.MayBecomeDistributor ())
		{
			// Distributor administrator cannot change his/her distributor status
			// because there are some other project members beside him/her
			_distributor.Disable ();
			_noBranching.Disable ();
		}
	}
	else if (_options.MayBecomeDistributor ())
	{
		_status.Hide ();
		_distributor.Enable ();
		_noBranching.Disable ();
		_allBcc.Check ();
		_allBcc.Disable ();
		_singleRecipient.Disable ();
	}
	else
	{
		_distributor.Hide ();
		_noBranching.Hide ();
		_frame.Hide ();
		_allBcc.Hide ();
		_singleRecipient.Hide ();
		_status.SetText ("You cannot become a distributor in this project.\n"
						 "You must be the only member of the project.");
	}
	if (_options.GetSeatTotal () != 0)
	{
		std::string license ("You have ");
		license += ToString (_options.GetSeatsAvailable ());
		license += " distribution licenses left out of total ";
		license += ToString (_options.GetSeatTotal ());
		license += " licenses assigned to ";
		license += _options.GetDistributorLicensee ();
		_license.SetText (license.c_str ());
	}
	else
	{
		_license.SetText ("You may purchase a distribution license over the Internet.");
	}
	return true;
}

bool ProjectDistributorCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_LICENSE_PURCHASE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
			int errCode = ShellMan::Open (appWnd, DistributorPurchaseLink);
			if (errCode != -1)
			{
				std::string msg = ShellMan::HtmlOpenError (errCode, "license", DistributorPurchaseLink);
				TheOutput.Display (msg.c_str (), Out::Error, GetWindow ());
			}
			else
			{
				PressButton (PropPage::Ok);
			}
		}
		return true;
	case IDC_PROJ_OPTIONS_DISTRIBUTOR:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			if (_distributor.IsChecked ())
			{
				_options.SetDistribution (true);
				_noBranching.Enable ();
				_allBcc.Enable ();
				_singleRecipient.Enable ();
				_options.SetAutoJoin (false);
			}
			else
			{
				_options.SetDistribution (false);
				_noBranching.Disable ();
				_allBcc.Disable ();
				_singleRecipient.Disable ();
			}
		}
		return true;
	}
    return false;
}

void ProjectDistributorCtrl::OnCancel (long & result) throw (Win::Exception)
{
	_options.Clear ();
}

void ProjectDistributorCtrl::OnApply (long & result) throw (Win::Exception)
{
	result = 0;		// Assume everything is OK
	if (_options.MayBecomeDistributor () || _options.IsDistribution ())
	{
		_options.SetDistribution (_distributor.IsChecked ());
		_options.SetNoBranching (_noBranching.IsChecked ());
		_options.SetBccRecipients (_allBcc.IsChecked ());
	}
}

bool ProjectEncryptionCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_isEncrypt.Init (dlgWin, IDC_PROJ_OPTIONS_ENCRYPT);
	_useCommonKey.Init (dlgWin, IDC_PROJ_OPTIONS_COMMON_PASS);
	_key.Init (dlgWin, IDC_PROJ_OPTIONS_ENCRYPT_PASS);
	_key2.Init (dlgWin, IDC_PROJ_OPTIONS_ENCRYPT_PASS2);
	_keyStatic.Init (dlgWin, IDC_PROJ_OPTIONS_STATIC);
	_key2Static.Init (dlgWin, IDC_PROJ_OPTIONS_STATIC2);
	InitializeControls ();
	return true;
}

bool ProjectEncryptionCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_PROJ_OPTIONS_ENCRYPT)
	{
		if (_isEncrypt.IsChecked ())
			EnableKeyControls ();
		else
			DisableKeyControls ();
	}
	else if (ctrlId == IDC_PROJ_OPTIONS_COMMON_PASS)
	{
		if (_useCommonKey.IsChecked ())
		{
			_key.SetText (_options.GetEncryptionCommonKey ());
			_key2.SetText (_options.GetEncryptionCommonKey ());
			_key.Disable ();
			_key2.Disable ();
		}
		else
		{
			_key.Enable ();
			_key2.Enable ();
			_key.Clear ();
			_key2.Clear ();
		}
	}
	return true;
}

void ProjectEncryptionCtrl::OnKillActive (long & result) throw (Win::Exception)
{
	result = 0;
	std::string key;
	std::string key2;
	if (_isEncrypt.IsChecked ())
	{
		key = _key.GetString ();
		key2 = _key2.GetString ();
		if (key.empty ())
		{
			TheOutput.Display ("Please specify the encryption key.");
			result = -1;
			return;
		}
		else if (key != key2)
		{
			TheOutput.Display ("Encryption keys do not match. Please re-enter the keys.");
			result = -1;
			return;
		}
	}

	std::string const originalKey = _options.GetEncryptionOriginalKey ();
	if (!originalKey.empty ())
	{
		if (key.empty ())
		{
			if (TheOutput.Prompt ("Are you sure you want to turn the encryption off?"
								 "\nYou will not be able to receive encoded scripts.") != Out::Yes)
			{
				result = -1;
				return;
			}
		}
		else if (key != originalKey)
		{
			if (TheOutput.Prompt ("Are you sure you want to change the encryption key?"
						  		"\n(You will not be able to receive scripts encrypted"
								"\nwith the old key.)") != Out::Yes)
			{
				result = -1;
				return;
			}
		}
	}
	_options.SetEncryptionKey (key);
}

void ProjectEncryptionCtrl::OnCancel (long & result) throw (Win::Exception)
{
	_options.Clear ();
}

void ProjectEncryptionCtrl::InitializeControls ()
{
	if (_options.GetEncryptionCommonKey ().empty ())
		_useCommonKey.Disable ();

	std::string const key = _options.GetEncryptionKey ();
	if (key.empty ())
	{
		_isEncrypt.UnCheck ();
		DisableKeyControls ();
	}
	else
	{
		_isEncrypt.Check ();
		EnableKeyControls ();
		_key.SetString (key);
		_key2.SetString (key);
	}
}

void ProjectEncryptionCtrl::DisableKeyControls ()
{
	_useCommonKey.Disable ();
	_keyStatic.Disable ();
	_key2Static.Disable ();
	_key.Disable ();
	_key2.Disable ();
}

void ProjectEncryptionCtrl::EnableKeyControls ()
{
	if (!_options.GetEncryptionCommonKey ().empty ())
		_useCommonKey.Enable ();

	_keyStatic.Enable ();
	_key2Static.Enable ();
	_key.Enable ();
	_key2.Enable ();
}

ProjectOptionsHndlrSet::ProjectOptionsHndlrSet (Project::OptionsEx & options)
	: PropPage::HandlerSet (options.GetCaption ()),
	  _options (options),
	  _optionsPageHndlr (options),
	  _distributorPageHndlr (options),
	  _encryptionPageHndlr (options)
{
	AddHandler (_optionsPageHndlr, "General");
	AddHandler (_distributorPageHndlr, "Distributor");

	// Notice: Encryption page is ready for use
	// AddHandler (_encryptionPageHndlr, "Encryption");
}

// command line
// -project_options autosynch:"on" or "off" autojoin:"on" or "off" keepcheckedout:"on" or "off"
bool ProjectOptionsHndlrSet::GetDataFrom (NamedValues const & source)
{
	std::string autoSyncValue;
	std::string autoJoinValue;
	 autoSyncValue = source.GetValue ("autosynch");
	 std::transform (autoSyncValue.begin (), autoSyncValue.end (), autoSyncValue.begin (), tolower);
	 autoJoinValue = source.GetValue ("autojoin");
	 std::transform (autoJoinValue.begin (), autoJoinValue.end (), autoJoinValue.begin (), tolower);
	_options.SetAutoSynch (autoSyncValue == "on");
	if (_options.IsAutoSynch ())
		_options.SetAutoJoin (autoJoinValue == "on");

	return !autoSyncValue.empty () || !autoJoinValue.empty ();
}
