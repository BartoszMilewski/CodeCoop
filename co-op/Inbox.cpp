//------------------------------------
//  (c) Reliable Software, 2003 - 2008
//------------------------------------

#include "precompiled.h"
#include "Model.h"
#include "AckBox.h"
#include "MailboxHelper.h"
#include "FileList.h"
#include "FileTrans.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "FeedbackMan.h"
#include "Conflict.h"
#include "ProjectMarker.h"
#include "Workspace.h"
#include "ScriptList.h"
#include "Registry.h"
#include "CtrlScriptProcessor.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "ScriptIo.h"
#include "CheckoutNotifications.h"
#include "ScriptBasket.h"

void Model::ProcessMail (bool force)
{
	if (_quickVisit)
		dbg << "Process Mail in project: " << _dataBase.ProjectName () << ", user id: " << std::hex << _dataBase.GetMyId () << std::endl;
	AckBox ackBox;
	if (UnpackScripts (ackBox, force))
	{
		SimpleMeter meter (_uiFeedback);
		ExecuteForcedScripts (ackBox, meter);
		meter.Close ();
		if (NextIsFullSynch ())
			_userPermissions.RefreshProjectData (GetProjectDb (), _pathFinder);
	}
	SendAcknowledgments (ackBox);

	// Executing forced scripts may remove us from the project if we have received defect from the administrator
	if (_pathFinder.IsInProject ())
	{
		bool newMissing = ProcessMissingScripts ();
		SetIncomingScriptsMarker (newMissing);
		ProcessScriptDuplicates ();
	}
}

// Returns true if there are script files in the inbox
bool Model::UnpackScripts (AckBox & ackBox, bool force)
{
	if (!_mailBox.ScriptFilesPresent () && !force)
		return false;

	// There are scripts present in the inbox -- unpack them
	// and select the ones ready to be executed immediately
	std::unique_ptr<CtrlScriptProcessor> ctrlScriptProcessor;
	CorruptedScriptMap corruptedScripts;
	bool retry = false;
	bool fullSyncCorruptionDetected = false;
	do
	{
		TransactionFileList unpackedScripts;
		Mailbox::Agent agent (unpackedScripts,
							  _userPermissions,
							  ackBox,
							  _dataBase.GetMyId (),
							  _activityLog);
		SimpleMeter meter (_uiFeedback);
		FileTransaction xact (*this, _pathFinder, unpackedScripts);
		//--------------------------------------------------

		if (_mailBox.XUnpackScripts (agent, corruptedScripts, &meter))
		{
			ctrlScriptProcessor.reset (new CtrlScriptProcessor (_mailer,
																_dataBase.XGetProjectDb (),
																_sidetrack));
			std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();
			// No corrupted scripts detected in the inbox
			agent.XFinalize (_mailBox, 
				_history, 
				_dataBase, 
				_checkedOutFiles, 
				*ctrlScriptProcessor,
				notification.get());
			retry = agent.IsPendingWork ();

			//----------------------------------------------
			xact.Commit ();

			if (agent.IsDeleteProjectVerificationMarker ())
			{
				BlockedCheckinMarker blockCheckin (_catalog, GetProjectId ());
				blockCheckin.SetMarker (false);
				RecoveryMarker recovery (_catalog, GetProjectId ());
				recovery.SetMarker (false);
			}
		}
		else
		{
			if (agent.IsFullSyncCorruptionDetected ())
				fullSyncCorruptionDetected = true;
			retry = true; // try again without the corrupted script.
			// Abort transaction (don't commit)
			//----------------------------------------------
		}
	} while (retry);

	if (fullSyncCorruptionDetected && !_sidetrack.HasChunks ())
	{
		// We have to remove the missing inventory placeholder.
		// We are returning to the state as if we've never seen a single full sync chunk.
		Transaction xact (*this, _pathFinder);
		_history.XRemoveMissingInventoryMarker ();
		xact.Commit ();
	}

	for (CorruptedScriptMap::const_iterator it = corruptedScripts.begin (); it != corruptedScripts.end (); ++it)
	{
		Mailbox::ScriptState state;
		state.SetForThisProject (true);
		state.SetCorrupted (true);
		_mailBox.RememberScript (it->first, state, it->second);
	}

	if (ctrlScriptProcessor.get () != 0 && ctrlScriptProcessor->NeedToForwardJoinRequest ())
	{
		if (!ForwardJoinRequest (ctrlScriptProcessor->GetJoinRequestPath ()))
		{
			PathSplitter splitter (ctrlScriptProcessor->GetJoinRequestPath ());
			std::string scriptFileName (splitter.GetFileName ());
			scriptFileName += splitter.GetExtension ();
			Mailbox::ScriptState state;
			state.SetForThisProject (true);
			state.SetJoinRequest (true);
			state.SetCannotForward (true);
			_mailBox.RememberScript (scriptFileName, state, "project administrator unknown");
		}
	}
	return true;
}

// Returns true if the new missing script/chunk entry has been added
bool Model::ProcessMissingScripts ()
{
	// New missing script is detected in the mailbox when we unpack the first script chunk or in
	// the history when we process the lineage of incoming script.
	// After unpacking the first chunk of a missing script, we set in the sidetrack the
	// new missing script flag and store in the history the missing script placeholder.
	// Both, the sidetrack and the history, immediately know about the missing script.
	// When we learn about a new missing script from the incoming script lineage, we place
	// the missing script placeholder in the history. At that time the sidetrack is unaware
	// of a new missing script. Later, when script unpacking completes, we retrieve
	// the missing script list from the history and pass it to the sidetrack for inspection.
	// At that moment the sidetrack learns about a new missing script.
	bool isNewMissing = false;
	Unit::ScriptList missingScripts;
	_history.GetMissingScripts (missingScripts);
	// Pass the missing script list from the history to the sidetrack for inspection.
	// If history knows more or less then the sidetrack, then the sidetrack needs updating.
	if (_sidetrack.NeedsUpdating (missingScripts))
	{
		// We have to generate new re-send request, or the missing script list from the
		// history have changed, or we have the full sync re-send requests.
		ScriptBasket scriptBasket;
		{
			Transaction xact (*this, _pathFinder);
			isNewMissing = _sidetrack.XProcessMissingScripts (missingScripts, scriptBasket);
			xact.Commit ();
		}
		std::unique_ptr<CheckOut::List> notification = GetCheckoutNotification ();
		scriptBasket.SendScripts(_mailer, notification.get(), _dataBase.GetProjectDb());

		while (_sidetrack.HasFullSynchRequests ())
		{
			try
			{
				FullSynchResend ();
			}
			catch (...)
			{
				Transaction xact (*this, _pathFinder);
				_sidetrack.XRemoveFullSynchChunkRequest ();
				xact.Commit ();
			}
		}
	}
	else
	{
		// We don't have to generate re-resend requests, and the sidetrack missing script list
		// is identical to the history missing script list, and we don't have the full sync
		// re-send requests.
		// Check if the sidetrack knows about a new missing script, because it just have stored
		// the missing script first chunk.
		isNewMissing = _sidetrack.IsNewMissing_Reset ();
	}
	return isNewMissing;
}

void Model::ProcessScriptDuplicates ()
{
	bool keepProcessing;
	do
	{
		keepProcessing = false;
		for (Mailbox::Db::Sequencer seq (_mailBox); !seq.AtEnd (); seq.Advance ())
		{
			Mailbox::ScriptInfo const & info = seq.GetScriptInfo ();
			if (info.IsIllegalDuplicate ())
			{
				// Found script in the in-box that has the same id as the one recorded in the history,
				// but has a different command list. If the script recorded in the history has not been
				// yet executed then ask the user if he/she wants to replace it with the one from the
				// in-box (script in the history may be corrupted and this the reason why it was not
				// executed yet).
				if (_history.IsCandidateForExecution (info.GetScriptId ()))
				{
					GlobalIdPack pack (info.GetScriptId ());
					std::string msg ("You have received another copy of the following script:\n\n");
					msg += pack.ToBracketedString ();
					msg += " - ";
					msg += info.Caption ();
					msg += "\n\nwhich is different from the copy already in your inbox.";
					msg += "\nIf you couldn't execute the first copy of the script";
					msg += "\nyou should replace it with the new copy.";
					msg += "\n\nDo you want to replace it?";
					Out::Answer userChoice = TheOutput.Prompt (msg.c_str (),
															   Out::PromptStyle (Out::YesNo,
																				 Out::No,
																				 Out::Question),
															   TheAppInfo.GetWindow ());
					if (userChoice == Out::Yes)
					{
						AckBox ackBox;
						TransactionFileList substitutedScripts;
						ScriptReader reader (info.GetScriptPath ());
						std::unique_ptr<ScriptHeader> hdr = reader.RetrieveScriptHeader ();
						std::unique_ptr<CommandList> cmds = reader.RetrieveCommandList ();

						Transaction xact (*this, _pathFinder);
						_history.XSubstituteScript (*hdr, *cmds, ackBox);
						_mailBox.XDeleteScript (info.GetScriptId ());
						xact.Commit ();

						keepProcessing = true;
						break;	// Restart mailbox processing, because one script has been removed from the in-box
					}
				}
			}
		}
	}
	while (keepProcessing);
}

void Model::SendAcknowledgments (AckBox const & ackBox)
{
	if (ackBox.empty ())
		return;

	std::unique_ptr<CheckOut::List> checkoutNotification = GetCheckoutNotification ();
	ScriptHeader ackHdr (ScriptKindAck (), gidInvalid, _dataBase.ProjectName ());
	// Acknowledgment box will assign real ack script id
	Transaction xact (*this, _pathFinder);

	_history.XGetLineages (ackHdr, UnitLineage::Minimal);
	ackBox.XSendAck (_mailer,
					 _dataBase.XGetProjectDb (),
					 ackHdr,
					 checkoutNotification.get ());

	xact.Commit ();
}

std::unique_ptr<CheckOut::List> Model::GetCheckoutNotification () const
{
	std::unique_ptr<CheckOut::List> notification;

	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	MemberState myState = projectDb.GetMemberState (projectDb.GetMyId ());
	if (myState.IsCheckoutNotification ())
	{
		GidList files;
		_dataBase.ListCheckedOutFiles (files);
		if (!files.empty ())
			notification.reset (new CheckOut::List (files));
	}

	return notification;
}

std::unique_ptr<CheckOut::List> Model::XGetCheckoutNotification ()
{
	std::unique_ptr<CheckOut::List> notification;

	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	MemberState myState = projectDb.XGetMemberState (projectDb.XGetMyId ());
	if (myState.IsCheckoutNotification ())
	{
		GidList files;
		_dataBase.XListCheckedOutFiles (files);
		if (!files.empty ())
			notification.reset (new CheckOut::List (files));
	}

	return notification;
}

void Model::BroadcastCheckoutNotification ()
{
	// Broadcast empty acknowledgement script to piggyback checkout notification
	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	MemberState myState = projectDb.GetMemberState (projectDb.GetMyId ());
	if (!myState.IsCheckoutNotification ())
		return;

	GidList broadcastList;
	projectDb.GetAllMemberList (broadcastList);
	if (broadcastList.empty ())
		return;

	std::unique_ptr<CheckOut::List> checkoutNotification = GetCheckoutNotification ();
	ScriptHeader ackHdr (ScriptKindAck (), gidInvalid, projectDb.ProjectName ());
	ackHdr.AddComment ("Checkout notification");
	CommandList emptyCmdList;

	{
		// Transaction scope
		Transaction xact (*this, _pathFinder);
		ackHdr.SetScriptId (_dataBase.XMakeScriptId ());
		_history.XGetLineages (ackHdr, UnitLineage::Minimal);
		xact.Commit ();
	}

	_mailer.Broadcast (ackHdr, emptyCmdList, checkoutNotification.get ());
}
