//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"
#include "ProgramOptionsDlg.h"
#include "OutputSink.h"
#include "Validators.h"
#include "BrowseForFolder.h"
#include "ProjectOptionsEx.h"

//
// Auto re-send options page
//

bool ResendPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_delaySelector.Init (GetWindow (), IDC_RESEND_OPTIONS_DELAY);
	_repeatSelector.Init (GetWindow (), IDC_RESEND_OPTIONS_REPEAT);

	unsigned int idx = 0;
	idx = _delaySelector.AddToList ("5 minutes");
	_delaySelector.SetItemData (idx, 5);
	idx = _delaySelector.AddToList ("10 minutes");
	_delaySelector.SetItemData (idx, 10);
	idx = _delaySelector.AddToList ("15 minutes");
	_delaySelector.SetItemData (idx, 15);
	idx = _delaySelector.AddToList ("20 minutes");
	_delaySelector.SetItemData (idx, 20);
	idx = _delaySelector.AddToList ("25 minutes");
	_delaySelector.SetItemData (idx, 25);
	idx = _delaySelector.AddToList ("half an hour");
	_delaySelector.SetItemData (idx, 30);
	idx = _delaySelector.AddToList ("one hour");
	_delaySelector.SetItemData (idx, 60);
	idx = _delaySelector.AddToList ("two hours");
	_delaySelector.SetItemData (idx, 120);
	idx = _delaySelector.AddToList ("three hours");
	_delaySelector.SetItemData (idx, 180);
	idx = _delaySelector.AddToList ("four hours");
	_delaySelector.SetItemData (idx, 240);
	idx = _delaySelector.AddToList ("five hours");
	_delaySelector.SetItemData (idx, 300);
	idx = _delaySelector.AddToList ("six hours");
	_delaySelector.SetItemData (idx, 360);
	idx = _delaySelector.AddToList ("half a day");
	_delaySelector.SetItemData (idx, 720);
	idx = _delaySelector.AddToList ("one day");
	_delaySelector.SetItemData (idx, 1440);

	idx = _repeatSelector.AddToList ("half an hour");
	_repeatSelector.SetItemData (idx, 30);
	idx = _repeatSelector.AddToList ("one hour");
	_repeatSelector.SetItemData (idx, 60);
	idx = _repeatSelector.AddToList ("two hours");
	_repeatSelector.SetItemData (idx, 120);
	idx = _repeatSelector.AddToList ("three hours");
	_repeatSelector.SetItemData (idx, 180);
	idx = _repeatSelector.AddToList ("four hours");
	_repeatSelector.SetItemData (idx, 240);
	idx = _repeatSelector.AddToList ("five hours");
	_repeatSelector.SetItemData (idx, 300);
	idx = _repeatSelector.AddToList ("six hours");
	_repeatSelector.SetItemData (idx, 360);
	idx = _repeatSelector.AddToList ("half a day");
	_repeatSelector.SetItemData (idx, 720);
	idx = _repeatSelector.AddToList ("one day");
	_repeatSelector.SetItemData (idx, 1440);

	ProgramOptions::Resend & resendOptions = _options._resend;
	unsigned currentDelay = resendOptions.GetDelay ();
	unsigned count = _delaySelector.Count ();
	for (idx = 0; idx < count; ++idx)
	{
		unsigned delay = _delaySelector.GetItemData (idx);
		if (currentDelay <= delay)
		{
			_delaySelector.SetSelection (idx);
			break;
		}
	}
	if (idx == count)
		_delaySelector.SelectString ("10 minutes");

	count = _repeatSelector.Count ();
	unsigned currentRepeat = resendOptions.GetRepeatInterval ();
	for (idx = 0; idx < count; ++idx)
	{
		unsigned repeat = _repeatSelector.GetItemData (idx);
		if (currentRepeat <= repeat)
		{
			_repeatSelector.SetSelection (idx);
			break;
		}
	}
	if (idx == count)
		_repeatSelector.SelectString ("one day");

	return true;
}

bool ResendPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	return true;
}

void ResendPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_options.ClearChanges ();
}

void ResendPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is OK
	ProgramOptions::Resend & resendOptions = _options._resend;
	unsigned int idx = _delaySelector.GetSelectionIdx ();
	unsigned int interval = _delaySelector.GetItemData (idx);
	resendOptions.SetDelay (interval);
	idx = _repeatSelector.GetSelectionIdx ();
	interval = _repeatSelector.GetItemData (idx);
	resendOptions.SetRepeat (interval);
}

//
// Update options page
//

void UpdatePageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_options.ClearChanges ();
}

void UpdatePageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is OK
	ProgramOptions::Update & updateOptions = _options._update;
	bool isAutoUpdate = _autoUpdate.IsChecked ();
	updateOptions.SetAutoUpdate (isAutoUpdate);
	if (isAutoUpdate)
	{
		updateOptions.SetBackgroundDownload (_inBackground.IsChecked ());

		int newPeriod = 0;
		_period.GetInt (newPeriod);
		UpdatePeriodValidator period (newPeriod);
		if (period.IsValid ())
		{
			updateOptions.SetUpdateCheckPeriod (newPeriod);
		}
		else
		{
			period.DisplayError ();
			result = -1;
		}
	}
}

bool UpdatePageHndlr::OnInitDialog () throw (Win::Exception)
{
	_autoUpdate.Init (GetWindow (), IDC_COOP_UPDATE_OPTIONS_AUTO_CHECK);
	_inBackground.Init (GetWindow (), IDC_COOP_UPDATE_OPTIONS_IN_BACKGROUND);
	_period.Init (GetWindow (), IDC_COOP_UPDATE_OPTIONSD_PERIOD_EDIT);
	_periodSpin.Init (GetWindow (), IDC_COOP_UPDATE_OPTIONS_PERIOD_SPIN);

	_periodSpin.SetRange (UpdatePeriodValidator::GetMin (), UpdatePeriodValidator::GetMax ());
	_period.LimitText (2);

	ProgramOptions::Update & updateOptions = _options._update;
	_periodSpin.SetPos (updateOptions.GetUpdateCheckPeriod ());
	if (updateOptions.IsBackgroundDownload ())
		_inBackground.Check ();
	else
		_inBackground.UnCheck ();
	if (updateOptions.IsAutoUpdate ())
	{
		_autoUpdate.Check ();
	}
	else
	{
		_autoUpdate.UnCheck ();
		_inBackground.Disable ();
		_period.Disable ();
		_periodSpin.Disable ();
	}

	return true;
}

bool UpdatePageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == IDC_COOP_UPDATE_OPTIONS_AUTO_CHECK && Win::SimpleControl::IsClicked (notifyCode))
	{
		if (_autoUpdate.IsChecked ())
		{
			_inBackground.Enable ();
			_period.Enable ();
			_periodSpin.Enable ();
		}
		else
		{
			_inBackground.Disable ();
			_period.Disable ();
			_periodSpin.Disable ();
		}
	} 
	return true;
}

//
// Chunking options page
//

void ChunkSizePageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_options.ClearChanges ();
}

void ChunkSizePageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is OK
	ProgramOptions::ChunkSize & chunkSizeOptions = _options._chunkSize;
	if (chunkSizeOptions.CanChange ())
	{
		unsigned newChunkSize = 0;
		bool validNumber = _size.GetUnsigned (newChunkSize);
		Assert (validNumber);	// Edit field marked as digits only
		ChunkSizeValidator validator (newChunkSize);
		if (validator.IsInValidRange ())
		{
			chunkSizeOptions.SetChunkSize (newChunkSize);
		}
		else
		{
			std::string info ("The script chunk size must be in the range from ");
			info += validator.GetMinChunkSizeDisplayString ();
			info += " to ";
			info += validator.GetMaxChunkSizeDisplayString ();
			TheOutput.Display (info.c_str ());
			result = 1;	// Don't close dialog
		}
	}
}

bool ChunkSizePageHndlr::OnInitDialog () throw (Win::Exception)
{
	_size.Init (GetWindow (), IDC_CHUNKING_OPTIONS_SIZE);
	_info.Init (GetWindow (), IDC_CHUNKING_OPTIONS_WARNING);

	ProgramOptions::ChunkSize & chunkSizeOptions = _options._chunkSize;
	std::string sizeStr (ToString (chunkSizeOptions.GetChunkSize ()));
	_size.SetString (sizeStr);

	if (chunkSizeOptions.CanChange ())
	{
		_info.Hide ();
	}
	else
	{
		_size.Disable ();
	}

	return true;
}

bool ChunkSizePageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	return true;
}

//
// Invitations
//

void InvitationsPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_options.ClearChanges ();
}

void InvitationsPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is OK
	ProgramOptions::Invitations & invitations = _options._invitations;
	if (_autoInvite.IsChecked ())
	{
		std::string autoInvitePath = _projectPath.GetString ();
		if (Project::OptionsEx::ValidateAutoInvite (autoInvitePath,
													invitations.GetCatalog (),
													GetWindow ()))
		{
			invitations.SetAutoInvite (true);
			invitations.SetAutoInvitePath (autoInvitePath);
		}
		else
		{
			result = 1;	// Don't close dialog
		}
	}
	else
	{
		invitations.SetAutoInvite (false);
	}
}

bool InvitationsPageHndlr::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_autoInvite.Init (dlgWin, IDC_OPTIONS_AUTO_INVITE);
	_projectPath.Init (dlgWin, IDC_OPTIONS_PATH);
	_pathBrowse.Init (dlgWin, IDC_OPTIONS_BROWSE);

	ProgramOptions::Invitations const & invitations = _options.GetInitationOptions ();
	_projectPath.SetString (invitations.GetProjectFolder ());
	if (invitations.IsAutoInvitation ())
	{
		_autoInvite.Check ();
		_projectPath.Enable ();
		_pathBrowse.Enable ();
	}
	else
	{
		_autoInvite.UnCheck ();
		_projectPath.Disable ();
		_pathBrowse.Disable ();
	}
	return true;
}

bool InvitationsPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (id == IDC_OPTIONS_AUTO_INVITE && Win::SimpleControl::IsClicked (notifyCode))
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
	else if (id == IDC_OPTIONS_BROWSE && Win::SimpleControl::IsClicked (notifyCode))
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

//
// Script Conflicts
//

void ScriptConflictPageHndlr::OnCancel (long & result) throw (Win::Exception)
{
	_options.ClearChanges ();
}

void ScriptConflictPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is OK
	_options._scriptConflict.SetResolveQuietly (_resolveQuietly.IsChecked ());
}

bool ScriptConflictPageHndlr::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin (GetWindow ());
	_resolveQuietly.Init (dlgWin, IDC_OPTIONS_RESOLVE_QUIETLY);
	if (_options._scriptConflict.IsResolveQuietly ())
		_resolveQuietly.Check ();
	else
		_resolveQuietly.UnCheck ();
	return true;
}

//
// Program options property sheet controllers
//

ProgramOptions::HandlerSet::HandlerSet (ProgramOptions::Data & programOptions)
	: PropPage::HandlerSet ("Code Co-op Options"),
	  _programOptions (programOptions),
	  _chunkSizePageHndlr (_programOptions),
	  _resendPageHndlr (_programOptions),
	  _scriptConflictPageHndlr (_programOptions),
	  _updatePageHndlr (_programOptions),
	  _invitationsPageHndlr (_programOptions)
{
	AddHandler (_chunkSizePageHndlr, "Script Chunking");
	AddHandler (_resendPageHndlr, "Missing Scripts");
	AddHandler (_scriptConflictPageHndlr, "Script Conflicts");
	AddHandler (_updatePageHndlr, "Co-op Update");
	AddHandler (_invitationsPageHndlr, "Invitations");
}
