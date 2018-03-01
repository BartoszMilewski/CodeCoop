//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include "precompiled.h"
#include "Commander.h"
#include "Model.h"
#include "ViewMan.h"
#include "DispatcherParams.h"
#include "Catalog.h"
#include "CollaborationSheet.h"
#include "ConfigWiz.h"
#include "EmailOptionsSheet.h"
#include "ConfigDlgData.h"
#include "ConfigExpt.h"
#include "AboutDlg.h"
#include "DiagDlg.h"
#include "PurgeDlg.h"
#include "SelectIter.h"
#include "OutputSink.h"
#include "Prompter.h"
#include "FeedbackMan.h"
#include "WizardHelp.h"
#include "Registry.h"
#include "Globals.h"
#include "DistributorTransferDlg.h"
#include "DistributorScriptSend.h"
#include "resource.h"

#include <LightString.h>
#include <Com/Shell.h>
#include <Sys/WinString.h>

void Commander::Program_CoopUpdate ()
{
	_msgCoopUpdate.SetWParam (0);
	_win.PostMsg (_msgCoopUpdate);
}

void Commander::Program_CoopUpdateOptions ()
{
	_msgCoopUpdate.SetWParam (1);
	_win.PostMsg (_msgCoopUpdate);
}

void Commander::Program_DistributorLicense ()
{
	Catalog & cat = _model.GetCatalog ();
	std::string licensee = cat.GetDistributorLicensee ();
	unsigned licCount = cat.GetDistributorLicenseCount ();
	unsigned nextLicNo = cat.GetNextDistributorNumber ();
	if (licCount == nextLicNo)
	{
		TheOutput.Display ("You have no distribution licenses to transfer");
		return;
	}
	RemoteHubList const & hubs = _model.GetRemoteHubList ();
	NocaseSet hubSet;
	for (RemoteHubList::Iterator it = hubs.begin (); it != hubs.end (); ++it)
		hubSet.insert (it->first);
	DistributorTransferData data (hubSet, licensee, licCount - nextLicNo);
	DistributorTransferCtrl ctrl (data);
	if (!ThePrompter.GetData (ctrl))
		return;

	std::string address;
	if (!data.IsLocalSave ())
	{
		if (hubSet.find (data.GetTargetHubId ()) == hubSet.end ())
		{
			address = data.GetTargetHubId ();
		}
		else
		{
			Transport const & transport = hubs.GetInterClusterTransport (data.GetTargetHubId ());
			address = transport.GetRoute ();
		}
		// revisit: use ValidateEmail common function
	}

	std::string instructions ("Please save the attachment, license.znc, "
		"and move it to the admin's machine if necessary. Once on the admin's machine, "
		"double click on the file license.znc to activate the license block.");

	MailOrSaveDistributorBlock (
		address,
		"PreviousAdmin",
		_model.GetConfigData ().GetHubId (),
		licensee,
		nextLicNo,
		licCount - nextLicNo,
		instructions,
		false); // send

	_model.GetCatalog ().ClearDistributorLicenses ();
}

Cmd::Status Commander::can_Program_DistributorLicense () const
{
#if defined (COOP_PRO)
	return Cmd::Enabled;
#else
	return Cmd::Invisible;
#endif
}

void Commander::Program_Collaboration ()
{
	_model.BeginConfigTransaction ();
	ConfigDlgData newCfg (_msgPrepro,
						  _model.XGetEmailMan (),
						  _model.XGetConfigData (),
						  _model.GetConfigData ());

	// Pass both newCfg and oldCfg to collaboration sheet
	CollaborationSheet sheet (_win, newCfg);
	if (sheet.Execute ())
	{
		throw ConfigExpt ();
	}
	else
	{
		_model.AbortConfigTransaction ();
	}
}

void Commander::Program_ConfigWizard ()
{
	_model.BeginConfigTransaction ();
	ConfigDlgData newCfg (_msgPrepro,
						  _model.XGetEmailMan (),
						  _model.XGetConfigData (),
						  _model.GetConfigData ());

	ConfigWizard wizard (newCfg);
	if (wizard.Execute ())
	{
		throw ConfigExpt ();
	}
	else
	{
		_model.AbortConfigTransaction ();
	}

	if (Registry::IsFirstRun ())
	{
		Win::ClearError ();
		throw Win::ExitException ("Configuration wizard aborted by the user.\n"
								  "Code Co-op cannot continue without proper configuration.\n\n"
								  "To repeat configuration process start Code Co-op again.");
	}
}

Cmd::Status Commander::can_Program_EmailOptions () const
{
	if (_model.IsConfigWithEmail ())
	{
		return Cmd::Enabled;
	}
	else
		return Cmd::Disabled;
}

void Commander::Program_EmailOptions ()
{
	// Revisit: Use ConfigTransaction
	_model.BeginConfigTransaction (); 
	ConfigData const & cfg = _model.XGetConfigData ();
	Topology topology = cfg.GetTopology ();
	std::string myEmail;
	if (topology.IsRemoteSatellite ())
		myEmail = cfg.GetActiveIntraClusterTransportToMe ().GetRoute ();
	else
		myEmail = cfg.GetInterClusterTransportToMe ().GetRoute ();
	Email::Manager & emailMan = _model.XGetEmailMan ();
	emailMan.BeginEdit ();
	if (Email::RunOptionsSheet (myEmail, emailMan, _win, _msgPrepro))
	{
		throw ConfigExpt ();
	}
	else
	{
		_model.AbortConfigTransaction ();
	}
}

void Commander::Program_Diagnostics ()
{
	DiagData diag (_msgPrepro,
				   TheEmail,
				   _model.GetClusterRecipients (),
				   _model.GetConfigData ());
    DiagCtrl ctrl (diag);
	ThePrompter.GetData (ctrl);
}

void Commander::Program_About ()
{
    AboutCtrl ctrl;
	ThePrompter.GetData (ctrl);
}

void Commander::Program_Exit ()
{
    _win.Destroy ();
}

void Commander::View_GoUp ()
{
	_viewMan->GoUp ();
}

Cmd::Status Commander::can_View_GoUp () const
{
	if (IsIn (ProjectMemberView))
	{
		return Cmd::Enabled;
	}
	else
		return Cmd::Disabled;
}

void Commander::View_Quarantine ()
{
	_viewMan->SelectTab (QuarantineView);
}

Cmd::Status Commander::can_View_Quarantine () const 
{
	return IsIn (QuarantineView) ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_AlertLog ()
{
	_viewMan->SelectTab (AlertLogView);
}

Cmd::Status Commander::can_View_AlertLog () const 
{
	return IsIn (AlertLogView) ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_PublicInbox ()
{
	_viewMan->SelectTab (PublicInboxView);
}

Cmd::Status Commander::can_View_PublicInbox () const 
{
	return IsIn (PublicInboxView) ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_Members ()
{
	_viewMan->SelectTab (MemberView);
}

Cmd::Status Commander::can_View_Members () const 
{
	return IsIn (MemberView) ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_RemoteHubs ()
{
	_viewMan->SelectTab (RemoteHubView);
}

Cmd::Status Commander::can_View_RemoteHubs () const 
{
	return IsIn (RemoteHubView) ? Cmd::Checked : Cmd::Enabled;
}

void Commander::View_Next ()
{
	_viewMan->SwitchToNextTab (true);
}

void Commander::View_Previous ()
{
	_viewMan->SwitchToNextTab (false);
}

void Commander::All_ReleaseFromQuarantine ()
{
	_model.ReleaseAllFromQuarantine ();
	_viewMan->Refresh (QuarantineView);
	_model.Dispatch (false); // don't force scattering
	RefreshUI ();
}

Cmd::Status Commander::can_All_ReleaseFromQuarantine () const
{
	if (IsIn (QuarantineView))
	{
		if (_model.HasQuarantineScripts ()) 
			return Cmd::Enabled;
	}

	return Cmd::Disabled;
}

void Commander::All_DispatchNow ()
{
    _model.ForceDispatch ();
	RefreshUI ();
}

Cmd::Status Commander::can_All_PullFromHub () const
{
	if (_model.IsConfigured () && !_model.IsHubOrPeer ())
		return Cmd::Enabled;
	else
		return Cmd::Disabled;
}

void Commander::All_PullFromHub ()
{
	_model.PullScriptsFromHub ();
}

Cmd::Status Commander::can_All_SendMail () const
{
	if (!_model.IsConfigured () || _model.IsSatellite ())
	{
		return Cmd::Disabled;
	}
	else
		return Cmd::Enabled;
}

void Commander::All_SendMail ()
{
	if (_model.IsConfigWithEmail ())
	{
		_model.RetrieveEmail ();
		_model.SendEmail();
		RefreshUI ();
	}
	else
	{
		ProposeUsingEmail ();
	}
}

Cmd::Status Commander::can_All_GetMail () const
{
	if (_model.IsSatellite ())
	{
		return Cmd::Disabled;
	}
	else
		return Cmd::Enabled;
}

void Commander::All_GetMail ()
{
	if (_model.IsConfigWithEmail ())
	{
		_model.RetrieveEmail();
	}
	else
	{
		ProposeUsingEmail();
	}
}

void Commander::ProposeUsingEmail ()
{
	if (Out::Yes == TheOutput.Prompt (
					"Would you like to configure the Dispatcher\n"
					"to start using e-mail for script distribution?",
					Out::PromptStyle (Out::YesNo)))
	{
		_model.StartUsingEmail ();
	}
}

void Commander::All_Purge ()
{
	PurgeData purgeData;
	if (_model.IsHub ())
	{
		purgeData.SetPurgeSatellite (true);
		PurgeCtrl ctrl (purgeData);
		if (!ThePrompter.GetData (ctrl) ||
			!(purgeData.IsPurgeLocal () || purgeData.IsPurgeSatellite ()))
		{
			return;
		}
	}

    if (_model.Purge (purgeData.IsPurgeLocal (), purgeData.IsPurgeSatellite ()))
	{
		_model.ForceDispatch ();
		RefreshUI ();
	}
}

void Commander::All_ViewAlertLog ()
{
	_model.ViewAlertLog ();
}

Cmd::Status Commander::can_All_ViewAlertLog () const
{
	if (IsIn (AlertLogView))
	{
		if (!_model.IsAlertLogEmpty ())
			return Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::All_ClearAlertLog ()
{
	_model.ClearAlertLog ();
	_viewMan->Refresh (AlertLogView);
}

Cmd::Status Commander::can_All_ClearAlertLog () const
{
	if (IsIn (AlertLogView))
	{
		if (!_model.IsAlertLogEmpty ())
			return Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::Selection_ReleaseFromQuarantine ()
{
	SelectionSeq seq (_viewMan, QuarantineTableName);
	Assert (!seq.AtEnd ());
	do
	{
		_model.ReleaseFromQuarantine (seq.GetName ());
		seq.Advance ();
	}while (!seq.AtEnd ());

	_viewMan->Refresh (QuarantineView);
	_model.Dispatch (false); // don't force scattering
	RefreshUI ();
}

Cmd::Status Commander::can_Selection_ReleaseFromQuarantine () const
{ 
	return (IsIn (QuarantineView) && _viewMan->HasSelection ()) ? 
		    Cmd::Enabled : Cmd::Disabled;
}

void Commander::Selection_Details ()
{
	if (IsIn (PublicInboxView))
	{
		Selection_HeaderDetails ();
	}
	else if (IsIn (MemberView))
	{
		Selection_ProjectMembers ();
	}
	else if (IsIn (ProjectMemberView))
	{
		Selection_EditTransport ();
	}
	else
	{
		Assert (IsIn (RemoteHubView));
		Selection_InterClusterTransport ();
	}
}

Cmd::Status Commander::can_Selection_Details () const
{ 
	if (IsIn (QuarantineView) || IsIn (AlertLogView))
		return Cmd::Disabled;

	return _viewMan->HasSelection () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Selection_HeaderDetails ()
{
	SelectionSeq seq (_viewMan, PublicInboxTableName);
	Assert (!seq.AtEnd ());
	_model.DisplayHeaderDetails (seq);
}

void Commander::Selection_ProjectMembers ()
{
	SelectionSeq seq (_viewMan, MemberTableName);
	Assert (!seq.AtEnd ());
	Restriction restrict (seq.GetName ());
	_viewMan->GoDown (&restrict);
}

void Commander::Selection_EditTransport ()
{
	SelectionSeq seq (_viewMan, ProjectMemberTableName);
	Assert (!seq.AtEnd ());

	if (_model.EditTransport (seq))
		RefreshUI ();
}

void Commander::Selection_InterClusterTransport ()
{
	SelectionSeq seq (_viewMan, RemoteHubTableName);
	Assert (!seq.AtEnd ());

	if (_model.EditInterClusterTransport (seq))
		RefreshUI ();
}

void Commander::Selection_Delete ()
{
	if (IsIn (PublicInboxView))
	{
		SelectionSeq seq (_viewMan, PublicInboxTableName);
		Assert (!seq.AtEnd ());
		DeleteSelectedScripts (seq);
	}
	else if (IsIn (ProjectMemberView))
	{
		SelectionSeq seq (_viewMan, ProjectMemberTableName);
		Assert (!seq.AtEnd ());
		if (_model.DeleteRecipients (seq))
			RefreshUI ();
	}
	else if (IsIn (QuarantineView))
	{
		SelectionSeq seq (_viewMan, QuarantineTableName);
		Assert (!seq.AtEnd ());
		DeleteSelectedScripts (seq, true); // all chunks
		_viewMan->Refresh (QuarantineView);
	}
	else
	{
		Assert (IsIn (RemoteHubView));
		SelectionSeq seq (_viewMan, RemoteHubTableName);
		Assert (!seq.AtEnd ());
		if (TheOutput.Prompt ("Are you sure you want to remove this remote hub from your list?", Out::YesNo)
			== Out::Yes)
		{
			_model.DeleteRemoteHubs (seq);
			RefreshUI ();
		}
	}
}

void Commander::DeleteSelectedScripts (SelectionSeq & seq, bool isAllChunks)
{
	while (!seq.AtEnd ())
	{
		_model.DeleteScript (seq.GetName (), isAllChunks);
		seq.Advance ();
	};
}

Cmd::Status Commander::can_Selection_Delete () const
{
	if (_viewMan->HasSelection ())
	{
		if (IsIn (QuarantineView) ||
			IsIn (PublicInboxView) || 
			IsIn (ProjectMemberView) || 
			IsIn (RemoteHubView))
		{
			return Cmd::Enabled;
		}
	}
	return Cmd::Disabled;
}

void Commander::Help_Contents ()
{
	OpenHelp ();
}

void Commander::Window_Show ()
{
	_win.PostMsg (_msgShowWindow);
}

bool Commander::IsIn (ViewType view) const
{
	return _viewMan->IsIn (view);
}

void Commander::RefreshUI ()
{
	_viewMan->RefreshAll ();
	TheFeedbackMan.RefreshScriptCount (_model.GetIncomingCount (), _model.GetOutgoingCount ());
}
