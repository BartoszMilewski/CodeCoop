//----------------------------------
// (c) Reliable Software 1998 - 2009
// ---------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "Model.h"
#include "Mailboxes.h"
#include "OutputSink.h"
#include "RecordSets.h"
#include "RecipKey.h"
#include "Dialogs.h"
#include "FwdPathChangeDlg.h"
#include "HubIdDlg.h"
#include "HeaderDlg.h"
#include "RemoteHubTransportDlg.h"
#include "Prompter.h"
#include "SelectIter.h"
#include "ClusterRecipient.h"
#include "resource.h"
#include "DispatcherParams.h"
#include "Addressee.h"
#include "ConfigDlgData.h"
#include "ConfigWiz.h"
#include "ConfigExpt.h"
#include "ProjectData.h"
#include "ScriptInfo.h"
#include "Processor.h"
#include "ScriptProcessorConfig.h"
#include "MailTruck.h"
#include "TransportData.h"
#include "UserIdPack.h"
#include "AlertMan.h"
#include "Globals.h"
#include "DispatcherScript.h"
#include "DispatcherCmd.h"
#include "TransportHeader.h"
#include "GlobalId.h"
#include "Serialize.h"
#include "Transport.h"
#include "RegKeys.h"
#include "RegFunc.h"
#include "ProjectMarker.h"
#include "Registry.h"
#include "SccProxy.h"
#include "AppInfo.h"
#include "DispatcherMsg.h"
#include "FeedbackMan.h"

#include <StringOp.h>
#include <Net/NetShare.h>
#include <Net/SharedObject.h>
#include <Com/Shell.h>
#include <File/File.h>
#include <File/Path.h>
#include <File/SafePaths.h>
#include <Ex/Error.h>
#include <LightString.h>

#include <lock.h>
#include <iomanip>

Model::Model (Win::Dow::Handle winParent,
		      Win::MessagePrepro & msgPrepro,
			  DispatcherCmdExecutor & cmdExecutor)
	: _config (_catalog),
	  _localProjects (_catalog),
	  _cmdExecutor (cmdExecutor),
	  _addressDb (_catalog, _config.GetHubId (), _config.GetTopology ().HasSat ()),
	  _winParent (winParent),
	  _isInboxFolderChanged (true)
{
	dbg << "Constructing Model." << std::endl;
	char const * logsPath = _catalog.GetLogsPath ().GetDir ();
	if (!File::Exists (logsPath))
	{
		// Recreate the Logs folder so that 
		// after user is done with logging we may ask him to delete the whole Logs folder
		File::CreateNewFolder (logsPath);
	}
	_alertLog.SetPath (logsPath);
	TheAlertMan.SetAlertLog (&_alertLog);

	_workQueue.SetWaitForDeath (10 * 1000);

	FilePath const & piPath = _config.GetPublicInboxPath ();
	if (piPath.IsDirStrEmpty ())
	{
		throw Win::ExitException ("The Public Inbox path is not defined. ",
								  "Please run Code Co-op Setup.");
	}
	else if (!File::Exists (piPath.GetDir ()))
	{
		throw Win::ExitException ("The Public Inbox folder does not exist. "
								  "Please run Code Co-op Setup.", 
								  piPath.GetDir ());
	}
	ResetPublicInbox ();

	_folderWatcher.reset (new FolderWatcher (_publicInbox->GetDir (), 
											 _winParent, 
											 false)); // non-recursive
	_folderWatcher.SetWaitForDeath (10 * 1000);

	TheTransportValidator.reset (new TransportValidator (
										_config.GetPublicInboxPath (),
										_config.GetIntraClusterTransportsToMe ()));				
}

void Model::ShutDown ()
{
	if (!_workQueue.empty())
		dbg << *_workQueue.get() << std::endl;
	_workQueue.reset ();
}

Model::~Model ()
{
	if (!_workQueue.empty())
		dbg << *_workQueue.get() << std::endl;
	TheTransportValidator.reset ();
}


//-------------------------------------------------
// Initialization
//-------------------------------------------------

void Model::OnStartup ()
{
	dbg << "--> Model::OnStartup." << std::endl;
	Assert (!Registry::IsFirstRun ());
	VerifyConfig ();

	_workQueue.reset (new WorkQueue (
		_config.GetOriginalData (), 
		&_publicInbox->GetScriptFileList(), 
		_winParent));

	_localProjects.Refresh ();

	Registry::DispatcherPrefs dispPrefs;
	unsigned long major, minor;
	if (!dispPrefs.GetVersion (major, minor))
	{
		dispPrefs.SetVersion (4, 5);
		if (_config.GetTopology ().IsHub ())
		{
			// First time: propagate max chunk size from hub to satellites
			Email::RegConfig const & emailCfg = TheEmail.GetEmailConfig ();
			BroadcastChunkSizeChange (emailCfg.GetMaxEmailSize ());
		}
	}
	if (_config.GetTopology ().UsesEmail())
		_emailChecker.reset(new EmailChecker(_winParent, _config.GetPublicInboxPath ()));
	dbg << "<-- Model::OnStartup." << std::endl;
}

void Model::VerifyConfig ()
{
	// ask about hub id
	if (!_config.GetTopology ().IsStandalone () &&
		_config.GetHubId ().empty ())
	{
		std::vector<std::string> idList;
		_addressDb.ListLocalHubIdsByImportance (idList);
		if (_config.GetTopology ().IsHub () && idList.empty ())
			_addressDb.ListClusterHubIdsByImportance (idList);

		std::string hubId;
		HubIdCtrl ctrl (idList, hubId, _config.GetTopology ().IsHubOrPeer ());
		if (ThePrompter.GetData (ctrl))
		{
			_config.BeginEdit ();
			ConfigData & newCfg = _config.XGetData ();
			newCfg.SetHubId (hubId);
			_config.CommitEdit ();
			_config.Save (_catalog);
		}
		else
			throw Win::ExitException ("The Code Co-op Dispatcher 4.x requires that the hub id is set.\nThe program will exit now.");
	}

	// check whether my transport path contains the current computer name
	Transport const myCurrentActiveTransport (_config.GetActiveIntraClusterTransportToMe ());
	if (myCurrentActiveTransport.IsNetwork ())
	{
		Assert (FilePath::IsValid (myCurrentActiveTransport.GetRoute ()));
		FilePath const myCurrentPath (myCurrentActiveTransport.GetRoute ());
		FullPathSeq seq (myCurrentPath.GetDir ());
		Assert (seq.IsUNC ());
		std::string const myCurrentComputerName (seq.GetServerName ());
		Assert (!myCurrentComputerName.empty ());
		std::string const systemComputerName (Registry::GetComputerName ());
		if (!systemComputerName.empty () && 
			!IsNocaseEqual (myCurrentComputerName, systemComputerName))
		{
			std::string myNewPath (myCurrentPath.ToString ());
			std::string::size_type startPos = myNewPath.find (myCurrentComputerName.c_str ());
			Assert (startPos != std::string::npos);
			myNewPath.replace (startPos, myCurrentComputerName.length (), systemComputerName.c_str ());

			_config.BeginEdit ();
			ConfigData & newCfg = _config.XGetData ();
			newCfg.SetActiveIntraClusterTransportToMe (Transport (myNewPath));
			_config.CommitEdit ();
			_config.Save (_catalog);

			TheAlertMan.PostInfoAlert ("The Dispatcher configuration contained different computer name "
									   "than the actual computer name of your system."
									   "\n\nThe Dispatcher configuration has been automatically updated.");

			BroadcastIntraClusterTransportToMeChange (myCurrentActiveTransport, _config.GetActiveIntraClusterTransportToMe ());
		}
	}
}

void Model::OnMaintenance (bool isOverall)
{
	if (isOverall)
	{
		_localProjects.ResendMissingScripts ();
	}
	else
	{
		_localProjects.Refresh ();
		HandleInvitationScripts ();
		KeepAlive ();
	}
}

void Model::HandleInvitationScripts ()
{
	std::vector<Address> inviteeList;
	_addressDb.GetInviteeList (inviteeList);
	if (inviteeList.empty ())
		return;

	std::string msg = "The processing of the invitation";
	if (inviteeList.size () == 1)
		msg += " to the project:\n\n";
	else
		msg += "s to the projects:\n\n";

	for (std::vector<Address>::const_iterator it = inviteeList.begin ();
		 it != inviteeList.end (); 
		 ++it)
	{
		Address const & invitee = *it;
		msg += invitee.GetProjectName ();
		msg += '\n';
		_addressDb.KillTempLocalRecipient (invitee);
	}
	msg += "\nwas unsuccessful.\nCode Co-op will give you the opportunity to process it"
		   " again\n(and to reject the invitation, if you cannot accept it).";
	TheOutput.Display (msg.c_str ());
	_cmdExecutor.PostForceDispatch ();
}

void Model::KeepAlive ()
{
	if (!_workQueue.IsAlive ())
	{
		TheAlertMan.PostInfoAlert ("Dispatcher worker thread died.\n\nRestarting.");

		_workQueue.reset (new WorkQueue (
			_config.GetOriginalData (), 
			&_publicInbox->GetScriptFileList(), 
			_winParent));

		Dispatch (false); // do not force scattering
	}
}

//-------------------------------------------------
// This is where processing takes place
//-------------------------------------------------

ProcessingResult Model::ProcessInbox (bool isForceScattering)
{
	BackupMarker backupMarker;
	if (backupMarker.Exists ())
		return ProcessingResult ();	// Don't process incoming scripts when restoring from backup

	if (!_workQueue.IsAlive ())
		return ProcessingResult ();

	MailTruck truck (_config.GetOriginalData (), _localProjects);
	_isInboxFolderChanged = false;
	// either copy locally or attach requests to ScriptTickets
	_publicInbox->ProcessScripts (truck, *_workQueue);

	{
		ScriptVector doneList;
		if (truck.GetDoneScripts (doneList))
		{
			dbg << "     MailTruck::GetDoneScripts returned " << doneList.size() << std::endl;
			FinishScripts (doneList);
			if (_publicInbox->GetScriptFileList().AcceptUiStatusChange())
			{
				Win::UserMessage msg (UM_SCRIPT_STATUS_CHANGE);
				_winParent.PostMsg (msg);
			}
		}
	}

	// Transfer the truckload to WorkQueue
	truck.Distribute (*_workQueue);

	if (isForceScattering)
		_workQueue->ForceScattering ();
    
	return ProcessingResult (!_isInboxFolderChanged, 
							 _localProjects.AreNewWithIncomingScripts ());
}

void Model::RetrieveEmail()
{
	if (_emailChecker.empty() || !_emailChecker.IsAlive())
		_emailChecker.reset(new EmailChecker(_winParent, _config.GetPublicInboxPath ()));

	_emailChecker->Force();
}

void Model::SendEmail ()
{
	Assert (IsConfigWithEmail ());
	ForceDispatch ();
}

void Model::Dispatch (bool isForceScattering)
{
	_isInboxFolderChanged = true;
	ProcessInbox (isForceScattering);
}

void Model::ForceDispatch ()
{
	_localProjects.Refresh ();
	ClearCurrentDispatchState ();
	Dispatch (true);
}

//-------------------------------------------------
// Notifications
//-------------------------------------------------

// return true, if public inbox is not already notified of change
bool Model::OnFolderChange ()
{
	std::string path;
	while (_folderWatcher->RetrieveChange (path))
		continue;

	bool wasChanged = _isInboxFolderChanged;
	_isInboxFolderChanged = true;
	return !wasChanged;
}

void Model::OnFinishScripts()
{
	if (!_workQueue.IsAlive ())
		return;
	dbg << "Model::OnFinishScripts" << std::endl;
	ScriptVector doneList;
	_workQueue->TransferDoneScripts (doneList);
	for (ScriptVector::iterator iter = doneList.begin (); 
		 iter != doneList.end (); ++iter)
	{
		ScriptTicket & script = **iter;
		if (script.IsEmailError ())
		{
			_scriptQuarantine.Insert (script.GetName (), "There was a problem e-mailing this script.");
		}		
		FinishScript(script);
	}
	_publicInbox->GetScriptFileList().AcceptUiStatusChange();
	// The caller will update UI anyway
}


void Model::FinishScripts (ScriptVector const & finishedList)
{
	for (ScriptVector::const_iterator it = finishedList.begin ();
		 it != finishedList.end (); ++it)
	{
		ScriptTicket const & script = **it;
		FinishScript(script);
	}
}

void Model::FinishScript(ScriptTicket const & script)
{
	dbg << "Model::FinishScript: " << script.GetName() << std::endl;
	if (script.CanDelete ()) // checks _stampedRecipients
	{
		dbg << "--delete--" << std::endl;
		File::DeleteNoEx (script.GetPath ());
		_workQueue->ForgetScript(script.GetName());
	}
	else if (_scriptQuarantine.FindModuloChunkNumber (script.GetName ()))
	{
		_publicInbox->GetScriptFileList().SetUiStatus (script.GetName(), ScriptStatus::Dispatch::Ignored);
	}
	else if (script.IsInvitation ())
	{
		// Waiting for co-op to create invitee's enlistment
	}
	else if (script.IsPostponed ())
	{
		// postpone dispatching till preceding scripts are fully processed
		// (e.g. invitation script fully processed and invitee enlistment created)
		// postponed scripts will be treated as new, seen for the first time, during 
		// every processing run until preceding scripts will be executed
	}
	else
	{
		DeleteOrIgnoreCtrl ctrl (script);
		if (ThePrompter.GetData (ctrl, "Undeliverable script"))
		{
			//DebugOut ("-Deleting: ");
			//DebugOut (script.GetPath ());
			//DebugOut ("\n");
			File::DeleteNoEx (script.GetPath ());
		}
		else
		{
			_scriptQuarantine.Insert (script.GetName (), "Script cannot be delivered to some recipients");
			_publicInbox->GetScriptFileList().SetUiStatus (script.GetName(), ScriptStatus::Dispatch::Ignored);
		}
	}
}

void Model::DisplayHeaderDetails (SelectionSeq & seq)
{
	while (!seq.AtEnd ())
	{
		std::string const & scriptName = seq.GetName ();
		std::unique_ptr<TransportData> tData = _publicInbox->GetTransportData (scriptName);

		char const * dlgText = 0;
		if (0 == tData.get ())
		{
			dlgText = "Could not open the script file.";
		}
		else if (!tData->IsPresent ())
		{
			dlgText = "This script has already been processed.";
		}
		else if (tData->IsEmpty ())
		{
			dlgText = "This script has no transport information.";
		}

		if (dlgText != 0)
		{
			std::string msg (dlgText);
			msg += "\n\n";
			msg += seq.GetName ();
			TheOutput.Display (msg.c_str ());
		}
		else
		{
			HeaderCtrl ctrl (*tData, seq.GetName ());
			ThePrompter.GetData (ctrl);
		}

		seq.Advance ();
	};
}

void Model::DeleteScript (std::string const & scriptName, bool isAllChunks)
{
	if (isAllChunks)
	{
		_publicInbox->DeleteScriptAllChunks (scriptName);
	}
	else
	{
		_publicInbox->DeleteScript (scriptName);
	}
	_scriptQuarantine.Delete (scriptName);
}

// Returns true if any recipient has been deleted from database
bool Model::DeleteRecipients (SelectionSeq & seq)
{
	bool anyRemoved = false;
	bool removedActiveSatUser = false;
	while (!seq.AtEnd ())
	{
		RecipientKey const & recip = static_cast<RecipientKey const &>(seq.GetKey ());
		Address address (recip.GetHubId (), seq.GetRestriction ().GetString (), recip.GetUserId ());

		if (recip.IsLocal ())
		{
			if (_addressDb.KillRemovedLocalRecipient (address))
			{
				anyRemoved = true;
			}
			else
			{
				TheOutput.Display ("You cannot directly remove an active local "
								   "user from the Dispatcher's database.\n"
								   "To remove this local user, use Code Co-op to "
								   "defect from this user's enlistment.");
			}
		}
		else
		{
			ClusterRecipient const * recip = 
					_addressDb.FindClusterRecipient (address);
			if (recip)
			{
				bool wantRemove = true;
				if (!recip->IsRemoved ())
				{
					Msg msg;
					UserIdPack idPack (address.GetUserId ());
					msg << "Are you sure you want to remove an active satellite user:\n"
						<< "project " << address.GetProjectName () 
						<< ", email address: " << address.GetHubId () 
						<< ", user id: " << idPack.GetUserIdString ()
						<< "\nfrom the Dispatcher's database ?";

					if (Out::Yes == TheOutput.Prompt (msg.c_str ()))
					{
						removedActiveSatUser = true;
					}
					else
					{
						wantRemove = false;
					}
				}
				if (wantRemove)
				{
					_addressDb.KillClusterRecipient (address);
					anyRemoved = true;
				}
			}
		}
		seq.Advance ();
	};

	if (removedActiveSatUser)
	{
		ForceDispatch ();
	}

	return anyRemoved;
}

void Model::AddRemoteHub (std::string const & hubId, Transport const & transport)
{
	if (!IsNocaseEqual (hubId, _catalog.GetHubId ()))
		_addressDb.AddInterClusterTransport (hubId, transport);
}

void Model::DeleteRemoteHubs (SelectionSeq & seq)
{
	while (!seq.AtEnd ())
	{
		_addressDb.DeleteRemoteHub (seq.GetName ());
		seq.Advance ();
	};
}

// Return true if any addresses removed
bool Model::Purge (bool purgeLocal, bool purgeSat)
{
	if (_addressDb.Purge (purgeLocal, purgeSat))
	{
		ClearCurrentDispatchState ();
		return true;
	}
	else
		return false;
}

//-------------------------------------------------
// Changes of settings
//-------------------------------------------------

void Model::EnlistmentListChanged ()
{
	if (_workQueue.IsAlive ())
		_workQueue->ClearState ();

	// must update the list of projects with pending scripts, 
	// also must remove a project that is being removed from
	// a list of projects to be notified
	// (it may happen there still is a pending notification for project!)
	_localProjects.Refresh ();
	
	_addressDb.LoadLocalRecips ();
	_isInboxFolderChanged = true;
}

void Model::EnlistmentEmailChanged ()
{
	if (_workQueue.IsAlive ())
		_workQueue->ClearState ();

	_addressDb.LoadLocalRecips ();
	_isInboxFolderChanged = true;
}

void Model::OnCoopNotification (TransportHeader & txHdr, DispatcherScript const & script)
{
	if (!_config.GetTopology ().HasHub ())
		return;

	// Write script to the public inbox, so my hub will process it
	txHdr.SetDispatcherAddendum (true);
	txHdr.SetForward (true);
	std::string myHubId = _catalog.GetHubId ();
	txHdr.AddSender (Address (myHubId.c_str (), txHdr.GetProjectName (), DispatcherAtSatId));
	// Create recipient list -- will contain only one recipient Dispatcher at HUB
	AddresseeList recipients;
	Addressee hubDispatcher (myHubId, DispatcherAtHubId);
	recipients.push_back (hubDispatcher);
	txHdr.AddRecipients (recipients);
	ScriptSubHeader comment (std::string ("Dispatcher-to-Dispatcher Notification"));
	// Create script file name
	// "xxxxxxxx To Hub Dispatcher.snc"
	Msg scriptName;
	scriptName << std::setw (8) << std::setfill ('0') << std::hex << txHdr.GetScriptId ();
	scriptName << " To Hub Dispatcher.snc";
	// Serialize script directly to the public inbox folder
	FilePath const publicInboxPath (_catalog.GetPublicInboxDir ());
	std::string const scriptPath = publicInboxPath.GetFilePath (scriptName.c_str ());
	FileSerializer out (scriptPath);
	txHdr.Save (out);
	comment.Save (out);
	script.Save (out);
}

void Model::StartUsingEmail ()
{
	_config.BeginEdit ();
	ConfigData & newCfg = _config.XGetData ();
	if (_config.GetTopology ().IsStandalone ())
	{
		newCfg.MakeHubWithEmail ();
	}
	else
	{
		newCfg.SetUseEmail (true);
	}
	throw ConfigExpt ();
}

void Model::Reconfigure (Win::MessagePrepro * msgPrepro)
{
	ConfigData & newConfig = _config.XGetData ();

	Registry::UserDispatcherPrefs prefs;
	if (Registry::IsFirstRun ())
	{
		// If first run, use different initialization
		// that doesn't assume previous configuration.
		ChangeSettings (); // <- commit changes to _config
		Registry::DispatcherUserRoot dispatcher;
		dispatcher.Key ().SetValueString ("ConfigurationState", "Configured");
		Transport const & newTransport = _config.GetActiveIntraClusterTransportToMe ();
		if (newTransport.IsNetwork ())
		{
			try // to change network sharing
			{
				Net::Share share;
				PathSplitter newShare (_config.GetActiveIntraClusterTransportToMe ().GetRoute ());
				Assert (newShare.IsValidShare ());
				Net::SharedFolder folder (newShare.GetFileName (), 
										  _config.GetPublicInboxPath ().GetDir (),
										  "Code Co-op Share");
				share.Add (folder);
			}
			catch (Win::Exception e)
			{
				TheOutput.Display (e);
				Win::ClearError ();
			}
		}

		Topology currentTopology = _config.GetTopology ();
		if (currentTopology.IsSatellite ())
			RegisterSatelliteOnHub ();

		// After configuring Dispatcher continue with Code Co-op
		CodeCoop::Proxy coopProxy;
		coopProxy.StartCodeCoop ();
		_cmdExecutor.Restart (500);
		return;
	}
	// Else detect configuration changes

	Topology oldTopology = _config.GetTopology ();
	Topology newTopology = newConfig.GetTopology ();
	std::string const oldHubId (_config.GetHubId ());
	std::string const newHubId (newConfig.GetHubId ());
	Transport const oldTransport (_config.GetActiveIntraClusterTransportToMe ());
	Transport const newTransport (newConfig.GetActiveIntraClusterTransportToMe ());
	Transport const oldActiveTransportToHub (_config.GetActiveTransportToHub ());
	Transport const newActiveTransportToHub (newConfig.GetActiveTransportToHub ());
	Transport const oldInterClusterTransportToMe (_config.GetInterClusterTransportToMe ());
	Transport const newInterClusterTransportToMe (newConfig.GetInterClusterTransportToMe ());

	bool const hubActiveTransportChanged = newActiveTransportToHub != oldActiveTransportToHub;

	unsigned long oldMaxSize = TheEmail.GetEmailConfig ().GetMaxEmailSize ();
	unsigned long newMaxSize = _config.XGetEmailMan ().GetEmailConfig ().GetMaxEmailSize ();

	ChangeSettings (); // <- commit changes to _config
	
	try // to change network sharing
	{
		if (newTopology.UsesNet() && !oldTopology.UsesNet())
		{
			Assert (newTransport.IsNetwork ());
			Net::Share share;
			PathSplitter newShare (_config.GetActiveIntraClusterTransportToMe ().GetRoute ());
			Assert (newShare.IsValidShare ());
			Net::SharedFolder folder (newShare.GetFileName (), 
									  _config.GetPublicInboxPath ().GetDir (),
									  "Code Co-op Public Inbox");
			share.Add (folder);
		}
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		Win::ClearError ();
	}
	_isInboxFolderChanged = true;

	if (oldTransport != newTransport)
	{
		BroadcastIntraClusterTransportToMeChange (oldTransport, newTransport);
	}

	if (oldHubId != newHubId || oldInterClusterTransportToMe != newInterClusterTransportToMe)
	{
		// Notify satellites of hubId / transport change
		BroadcastInterClusterTransportToMeChange (oldHubId, newHubId, newInterClusterTransportToMe);
	}

	if (oldTopology == newTopology)
	{
		if (newTopology.IsSatellite () && hubActiveTransportChanged) // sat changes transport to hub
		{
			Assert (oldActiveTransportToHub.IsNetwork ());
			Assert (newActiveTransportToHub.IsNetwork ());
			FullPathSeq oldHubPath (oldActiveTransportToHub.GetRoute ().c_str ());
			FullPathSeq newHubPath (newActiveTransportToHub.GetRoute ().c_str ());
			if (!IsNocaseEqual(oldHubPath.GetServerName(), newHubPath.GetServerName())
				|| !IsNocaseEqual(oldHubId, newHubId))
			{
				PublishEnlistmentsToHub (oldHubId, newHubId);
			}
		}

		if ((oldTopology.UsesEmail () != newTopology.UsesEmail ()) ||   // changed e-mail usage
			(newTopology.HasHub () && hubActiveTransportChanged))       // changed transport to hub
		{
			ClearCurrentDispatchState ();
		}
	}
	else
	{
		if (!oldTopology.IsSatellite () && newTopology.IsSatellite ()) // this machine becomes a sat
		{
			RegisterSatelliteOnHub ();
		}

		ResetInfrastructure ();
	}

	// hub id change
	if (!IsNocaseEqual (oldHubId, _config.GetHubId ()))
		_addressDb.HubIdChanged (oldHubId, _config.GetHubId ());

	if (Registry::IsRestoredConfiguration ())
	{
		// After restoring project database from the archive backup
		// the user successfully verified Dispatcher configuration.
		Registry::DispatcherUserRoot dispatcher;
		dispatcher.Key ().SetValueString ("ConfigurationState", "Configured");
	}

	if (oldMaxSize != newMaxSize)
	{
		Win::RegisteredMessage msgChunkSize (UM_COOP_CHUNK_SIZE_CHANGE);
		msgChunkSize.SetWParam (newMaxSize);
		TheAppInfo.GetWindow ().PostMsg (msgChunkSize);
	}
	if (oldTopology.UsesEmail () != newTopology.UsesEmail ())
	{
		if (newTopology.UsesEmail())
			_emailChecker.reset(new EmailChecker(_winParent, _config.GetPublicInboxPath ()));
		else
			_emailChecker.reset();
	}
}

void Model::ChangeSettings ()
{
	_config.CommitEdit ();
	_config.Save (_catalog);
	if (_workQueue.IsAlive ())
		_workQueue->ChangeConfig (_config.GetOriginalData ());
}

void Model::PullScriptsFromHub ()
{
	std::unique_ptr<DispatcherCmd> pullFromHubCmd (new ForceDispatchCmd);
	SendDispatcherCommand (std::move(pullFromHubCmd), DispatcherAtSatId, DispatcherAtHubId);
}

void Model::BroadcastIntraClusterTransportToMeChange (Transport const & oldTrans, Transport const & newTrans) const
{
	if (_config.GetTopology ().IsStandalone () || _config.GetTopology ().IsPeer ())
		return;

	std::unique_ptr<DispatcherCmd> replaceTransport (
		new ReplaceTransportCmd (oldTrans, newTrans));
	if (_config.GetTopology ().HasHub ())
	{
		// a satellite or a remote satellite or a proxy hub must notify its hub
		SendDispatcherCommand (std::move(replaceTransport), DispatcherAtSatId, DispatcherAtHubId);
	}
	else if (_config.GetTopology ().IsHub ())
	{
		// a hub must notify its satellites
		SendDispatcherCommand (std::move(replaceTransport), DispatcherAtHubId, DispatcherAtSatId);
	}
}

void Model::BroadcastChunkSizeChange (unsigned long newChunkSize)
{
	if (_config.GetTopology ().IsHub ())
	{
		std::unique_ptr<DispatcherCmd> chunkSizeCmd (
			new ChunkSizeCmd (newChunkSize));
		// a hub must notify its satellites
		SendDispatcherCommand (std::move(chunkSizeCmd), DispatcherAtHubId, DispatcherAtSatId);
	}
}

void Model::BroadcastInterClusterTransportToMeChange (std::string const & oldHubId, 
											   std::string const & newHubId, 
											   Transport const & newTrans) const
{
	if (!_config.GetTopology ().IsHubOrPeer ())
		return;
	// Hub notifies satellites of hub id change (they have to update their hub id)
	std::unique_ptr<DispatcherCmd> replaceHubId (
		new ChangeHubIdCmd (oldHubId, newHubId, newTrans));
	SendDispatcherCommand (std::move(replaceHubId), DispatcherAtHubId, DispatcherAtSatId);
}

void Model::RegisterSatelliteOnHub () const
{
	Transport const & networkTransport = _config.GetIntraClusterTransportsToMe ().Get (Transport::Network);
	Assert (!networkTransport.GetRoute ().empty ());
	std::unique_ptr<DispatcherCmd> addMemberCmd (
		new AddMemberCmd (_config.GetHubId (), 
						  std::string (), 
						  networkTransport));

	SendDispatcherCommand (std::move(addMemberCmd), DispatcherAtSatId, DispatcherAtHubId);
}

void Model::PublishEnlistmentsToHub (std::string const & oldHubId, std::string const & newHubId) const
{
	// Notice: both active and removed entries are published
	Transport const & activeTransport = _config.GetActiveIntraClusterTransportToMe ();
	Assert (!activeTransport.GetRoute ().empty ());
	std::unique_ptr<AddSatelliteRecipientsCmd> addMemberListCmd (
		new AddSatelliteRecipientsCmd (activeTransport));

	bool isSameHubId = IsNocaseEqual(oldHubId, newHubId);
	LocalRecipientList const & localEnlistments = _addressDb.GetLocalRecipients ();
	for (LocalRecipientList::const_iterator it = localEnlistments.begin ();
		 it != localEnlistments.end ();
		 ++it)
	{
		if (isSameHubId)
		{
			addMemberListCmd->AddMember (*it, it->IsRemoved ());
			continue;
		}
		// Hub ID changed
		Address newAddress = *it;
		newAddress.SetHubId(newHubId);
		addMemberListCmd->AddMember (newAddress, it->IsRemoved ());
		addMemberListCmd->AddMember (*it, true); // remove the old address, if any
	}

	SendDispatcherCommand (
			std::move(addMemberListCmd), 
			DispatcherAtSatId, 
			DispatcherAtHubId);
}

void Model::SendDispatcherCommand (std::unique_ptr<DispatcherCmd> cmd, char const * senderId, char const * addresseeId) const
{
	std::string const & hubId = _config.GetHubId ();
	FilePath const & outDir = _config.GetPublicInboxPath ();
	std::string fileName;
	SaveDispatcherScript (std::move(cmd), senderId, addresseeId, hubId, outDir, fileName);
}

void Model::OnConnectProxy (bool connected)
{
	if (!_config.GetTopology ().IsTemporaryHub ())
		return;

	// we are a proxy hub
	if (connected)
	{
		if (!_config.AskedToStayOffSiteHub ())
		{
			// re-entrancy protection
			static bool isAskingNow = false;
			if (isAskingNow)
				return;

			ReentranceLock switching2Sat (isAskingNow);

			_config.BeginEdit ();
			ConfigData & newCfg = _config.XGetData ();
			if (Out::Yes == TheOutput.Prompt ("Would you like to switch back from Off-Site Hub "
											  "to Satellite configuration?\n"
											  "(The Dispatcher has detected that you are now "
											  "connected to the hub.)\n\n"
											  "If the copying of scripts is still in progress,\n"
											  "wait until it completes!"))
			{
				newCfg.MakeSatellite ();
			}
			else
			{
				newCfg.SetAskedToStayOffSiteHub (true);
			}
			throw ConfigExpt ();
		}
	}
	else // disconnected
	{
		if (_config.AskedToStayOffSiteHub ())
		{
			_config.BeginEdit ();
			ConfigData & newCfg = _config.XGetData ();
			newCfg.SetAskedToStayOffSiteHub (false);
			throw ConfigExpt ();
		}
	}
}

bool Model::OnCoopTimer ()
{
	return _localProjects.UnpackIncomingScripts ();
}

void Model::ResetInfrastructure ()
{
	ClearCurrentDispatchState ();

	ResetPublicInbox ();
	if (!_workQueue.empty())
		_workQueue->ResetScriptFileList(&_publicInbox->GetScriptFileList());

	if (_config.GetTopology ().IsHub ())
	{
		_addressDb.LoadClusterRecips ();
	}
	else
	{
		_addressDb.ClearClusterRecips ();
	}
}

void Model::ResetPublicInbox ()
{
	FilePath const & piPath = _config.GetPublicInboxPath ();
    if (_config.GetTopology ().IsStandalone ())
	{
        _publicInbox.reset (new StandalonePublicInbox (
									piPath,
									_addressDb,
									_scriptQuarantine,
									_config,
									_cmdExecutor));
	}
    else if (_config.GetTopology ().IsHubOrPeer ())
	{
        _publicInbox.reset (new HubPublicInbox (
									piPath,
									_addressDb,
									_scriptQuarantine,
									_config,
									_cmdExecutor));
	}
	else if (_config.GetTopology ().IsTemporaryHub ())
	{
        _publicInbox.reset (new ProxyHubPublicInbox (
									piPath,
									_addressDb,
									_scriptQuarantine,
									_config,
									_cmdExecutor));
	}
    else
	{
		Assert (_config.GetTopology ().IsSatellite () ||
				_config.GetTopology ().IsRemoteSatellite ());

        _publicInbox.reset (new SatPublicInbox (
									piPath,
									_addressDb,
									_scriptQuarantine,
									_config,
									_cmdExecutor));
	}
}

void Model::ClearCurrentDispatchState ()
{
	_scriptQuarantine.Clear ();
	if (_workQueue.IsAlive ())
	{
		_workQueue->ClearState ();
		_workQueue->ClearEmailBlacklist ();
		_workQueue->UnIgnore ();
	}
}

std::string Model::FindSatellitePath (std::string const & computerName) const
{
	Assert (_config.GetTopology ().IsHub ());
	return _addressDb.FindSatellitePath (computerName);
}

void Model::AddClusterRecipient (Address const & address, FilePath const & forwardingPath)
{
	Assert (IsNocaseEqual (address.GetHubId (), _config.GetHubId ()));
	Transport newRecipTransport (forwardingPath);
	Assert (newRecipTransport.IsNetwork ());
	_addressDb.AddClusterRecipient (address, newRecipTransport);
}

// Returns true if path is changed
bool Model::EditTransport (SelectionSeq const & seq)
{
	RecipientKey const & key = static_cast<RecipientKey const &>(seq.GetKey ());
	if (key.IsLocal ())
	{
		TheOutput.Display ("You cannot change the transport path of a local user.");
		return false;
	}

	Address address (key.GetHubId (), seq.GetRestriction ().GetString (), key.GetUserId ());

	ClusterRecipient const * cr = _addressDb.FindClusterRecipient (address);
	Assert (cr);
	if (cr->IsRemoved ())
	{
		TheOutput.Display ("You cannot change the transport of a removed satellite user.");
		return false;
	}

	Transport oldTransport (cr->GetTransport ());
	FwdPathChangeData data (address,
							oldTransport,
							_addressDb.CountTransportUsers (oldTransport) > 1);
	FwdPathChangeCtrl ctrl (data);
	if (ThePrompter.GetData (ctrl))
	{
		Transport const & newTransport = data.GetNewTransport ();
		if (newTransport == oldTransport)
			return false;

		if (data.ReplaceAll ())
			_addressDb.ReplaceTransport (oldTransport, newTransport);
		else
			_addressDb.UpdateTransport (address, newTransport);

		ForceDispatch ();
		return true;
	}
	return false;
}

// Returns true if transport is changed
bool Model::EditInterClusterTransport (SelectionSeq const & seq)
{
	std::string const & hubId = seq.GetName ();
	Transport oldTransport (_addressDb.GetInterClusterTransport (hubId));
	InterClusterTransportData data (hubId, oldTransport);
	InterClusterTransportCtrl ctrl (data);
	if (ThePrompter.GetData (ctrl))
	{
		Transport const & newTransport = data.GetNewTransport ();
		if (newTransport == oldTransport)
			return false;

		_addressDb.UpdateInterClusterTransport (hubId, newTransport);

		ForceDispatch ();
		return true;
	}
	return false;
}

std::unique_ptr<RecordSet> Model::Query (std::string const & name, Restriction const & restrict)
{
	if (PublicInboxTableName == name)
	{
        return std::unique_ptr<RecordSet> (new PublicInboxRecordSet (*_publicInbox.get ()));
	}
	else if (MemberTableName == name)
	{
        return std::unique_ptr<RecordSet> (new MemberRecordSet (_addressDb));
	}
	else if (ProjectMemberTableName == name)
	{
        return std::unique_ptr<RecordSet> (new ProjectMemberRecordSet (_addressDb, restrict));
	}
	else if (RemoteHubTableName == name)
	{
        return std::unique_ptr<RecordSet> (new RemoteHubRecordSet (_addressDb.GetRemoteHubList ()));
	}
	else if (QuarantineTableName == name)
	{
		return std::unique_ptr<RecordSet> (new QuarantineRecordSet (_scriptQuarantine));
	}
	else if (AlertLogTableName == name)
	{
		return std::unique_ptr<RecordSet> (new AlertLogRecordSet (_alertLog));
	}
	else
	{
		Assert (!"Unknown table name");
		// unreachable
		return std::unique_ptr<RecordSet> ();
	}
}

std::string Model::QueryCaption (std::string const & tableName, Restriction const & restrict) const
{
	if (PublicInboxTableName == tableName)
	{
		return std::string ();
	}
	else if	(MemberTableName  == tableName)
	{
		Assert (!restrict.IsNumberSet ());
		Assert (!restrict.IsStringSet ());
		return _addressDb.QueryCaption (restrict);
	}
	else if	(ProjectMemberTableName  == tableName)
	{
		Assert (restrict.IsStringSet ());
		return _addressDb.QueryCaption (restrict);
	}
	else if	(RemoteHubTableName  == tableName)
	{
		return std::string ();
	}
	else if	(QuarantineTableName  == tableName)
	{
		return std::string ();
	}
	else if	(AlertLogTableName  == tableName)
	{
		return std::string ();
	}
	else
	{
		Assert (!"Invalid table name");
		return std::string ();
	}
}

bool Model::OnProjectStateChange (int projectId)
{
	return _localProjects.OnStateChange (projectId);
}

int Model::GetIncomingCount () const
{
	return _localProjects.GetToBeSynchedCount ();
}

int Model::GetOutgoingCount () const
{
	return _publicInbox->GetScriptCount ();
}

// 1. There is an old license
//   1.1 Under different name
//      1.1.1 Spent -> overwrite old license
//      1.1.2 Still usable -> Error
//   1.2 Under the same name
//      1.1.1 newStart == oldCount -> use (old start, oldCount + newCount)
//      1.1.2 newStart > oldCount -> OK, use (newStart, newCount + newStart)
//      1.1.3 newStart < oldCount -> Error 
// 2. There is no old license -> overwrite

void Model::AddDistributorLicense (std::string const & licensee, 
								  unsigned startNum, 
								  unsigned count)
{
	if (licensee != _catalog.GetLicensee () && licensee != _catalog.GetDistributorLicensee ())
		throw Win::Exception ("Distribution license not for this user");
	std::string const & oldLicensee = _catalog.GetDistributorLicensee ();
	unsigned oldStart = 0;
	unsigned oldCount = 0;
	if (oldLicensee.empty ())
	{
		_catalog.SetDistributorLicense (licensee, startNum, count);
		TheAlertMan.PostInfoAlert ("Accepting a block of distribution licenses");
	}
	else
	{
		oldStart = _catalog.GetNextDistributorNumber ();
		oldCount = _catalog.GetDistributorLicenseCount ();
		if (oldLicensee != licensee)
		{
			if (oldStart < oldCount)
			{
				throw Win::InternalException ("Could't accept new license block.\n"
					"You still have unused distribution licenses assigned to:", 
					oldLicensee.c_str ());
			}
			Assert (oldStart >= oldCount);
			_catalog.SetDistributorLicense (licensee, startNum, count);
			TheAlertMan.PostInfoAlert ("Replacing old, used up, distribution license block");
		}
		else // the same licensee
		{
			if (startNum < oldCount)
			{
				throw Win::InternalException ("Couldn't accept an incorrect (overlapping) distribution license block");
			}
			Assert (startNum >= oldCount);
			if (startNum > oldCount)
			{
				_catalog.SetDistributorLicense (licensee, startNum, startNum + count);
				TheAlertMan.PostInfoAlert ("Replacing an old block of distribution licenses with a new block");
			}
			else
			{
				Assert (startNum == oldCount); // nice extension
				_catalog.SetDistributorLicense (licensee, oldStart, oldCount + count);
				TheAlertMan.PostInfoAlert ("Extending the old block of distribution licenses");
			}
		}
	}
}

void Model::ReleaseAllFromQuarantine ()
{
	_scriptQuarantine.Clear ();
	if (_workQueue.IsAlive ())
		_workQueue->ClearEmailBlacklist ();
}

void Model::ReleaseInvitationFromQuarantine ()
{
	_scriptQuarantine.RemoveInvitations();
	ProcessInbox(false);
}

void Model::ReleaseFromQuarantine (std::string const & scriptFilename)
{
	_scriptQuarantine.Delete (scriptFilename);
	// Revisit: only addressees of this particular script should be removed from the blacklist
	if (_workQueue.IsAlive ())
		_workQueue->ClearEmailBlacklist ();
}
