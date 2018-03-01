//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Model.h"
#include "ScriptHeader.h"
#include "CmdExec.h"
#include "ScriptCommandList.h"
#include "ScriptList.h"
#include "Conflict.h"
#include "Transformer.h"
#include "FileList.h"
#include "FileTrans.h"
#include "SynchTrans.h"
#include "Catalog.h"
#include "Registry.h"
#include "License.h"
#include "PhysicalFile.h"
#include "SelectIter.h"
#include "OutputSink.h"
#include "TmpProjectArea.h"
#include "ConflictDlg.h"
#include "DispatcherProxy.h"
#include "FeedbackMan.h"
#include "TransportHeader.h"
#include "DispatcherScript.h"
#include "ProjectChecker.h"
#include "Predicate.h"
#include "Workspace.h"
#include "AppInfo.h"
#include "Messengers.h"
#include "FileTyper.h"
#include "ScriptTrailer.h"
#include "AckBox.h"
#include "JoinContext.h"
#include "MembershipUpdateVerifier.h"
#include "MembershipChange.h"
#include "AltDiffer.h"
#include "BuiltInMerger.h"

#if !defined (NDEBUG)
#include "MemoryLog.h"
#endif


#include <Ctrl/ProgressMeter.h>
#include <Dbg/Out.h>

#include <iomanip>

//
// Synch operations
//

// Returns true if no merge conflicts
bool Model::AcceptSynch (WindowSeq & seq)
{
	dbg << "Model::AcceptSynch" << std::endl;
	bool isConflict = false;
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		dbg << " file GID: " << GlobalIdPack(gid).ToBracketedString() << std::endl;
		Assert (gid != gidInvalid);
		Transformer trans (_dataBase, gid);
		FileState state = trans.GetState ();
		if (state.IsMergeConflict ())
		{
			dbg << "      --- merge conflict ----" << std::endl;
			isConflict = true;
			continue;
		}
		trans.AcceptSynch (_pathFinder, fileList, _synchArea);
		_directory.Notify (changeEdit, gid);
		if (trans.IsCheckedOut ())
			_checkInArea.Notify (changeEdit, gid);
	}
	xact.Commit ();
	SetIncomingScriptsMarker ();
	dbg << "Model::AcceptSynch returning " << !isConflict << std::endl;
	return !isConflict;
}

void Model::AcceptSynch (Progress::Meter & meter)
{
	if (_synchArea.IsEmpty ())
		return;

	GidList files;
	_synchArea.GetFileList (files);
	meter.SetRange (0, files.size ());
	meter.SetActivity ("Accepting synchronization script.");
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);
	for (GidList::const_iterator iter = files.begin (); iter != files.end (); ++iter)
	{
		meter.StepIt ();
		GlobalId gid = *iter;
		Transformer trans (_dataBase, gid);
		trans.AcceptSynch (_pathFinder, fileList, _synchArea);
		_directory.Notify (changeEdit, gid);
		if (trans.IsCheckedOut ())
			_checkInArea.Notify (changeEdit, gid);
	}
	xact.Commit ();
	SetIncomingScriptsMarker ();
}

//
// Script operations
//

// Returns true if file script executed successfully
bool Model::ExecuteSetScript (ScriptConflictDlg & conflictDialog, 
							  Progress::Meter & overallMeter,
							  Progress::Meter & specificMeter,
							  bool & beginnerHelp)
{
	dbg << "--> Model::ExecuteSetScript" << std::endl;
	if (IsQuickVisit ())
	{
		if (!_synchArea.IsEmpty () || !_userPermissions.CanExecuteSetScripts ())
			return false;
	}
	else
	{
		if (!_synchArea.IsEmpty ("proceeding with the current script"))
			return false;

		if (!_userPermissions.CanExecuteSetScripts ())
		{
			TheOutput.Display ("You cannot execute incoming script(s), because your Code Co-op trial period has just ended\n"
							   "and the project administrator didn't send you a valid license.");
			return false;
		}
	}

	ScriptHeader hdr;
	CommandList cmdList;
	bool isForceAccept = _history.RetrieveNextScript (hdr, cmdList);
	Assert (hdr.IsSetChange ());
	VerifySender (hdr);
	std::unique_ptr<ConflictDetector> detector (new ConflictDetector (_dataBase, _history, hdr));
	if (detector->AreMyScriptsRejected () || detector->AreMyLocalEditsInMergeConflict ())
	{
		if (IsQuickVisit ())
		{
			// Don't unpack script
			return false;
		}
		else
		{
			// In GUI show dialog
			if (!conflictDialog.Show (*detector))
				return false;	// User canceled script conflict dialog -- abort script unpacking

			bool isAutoMerge = detector->IsAutoMerge ();
			// Recreate detector in case something changed during conflict dialog display
			detector.reset (new ConflictDetector (_dataBase, _history, hdr));
			detector->SetAutoMerge (isAutoMerge);
		}
	}
	else if (detector->IsConflict ())
	{
		detector->SetAutoMerge (false);
		if (!IsQuickVisit () && !Registry::IsQuietConflictOption ())
		{
			TheOutput.Display (
				"The sender of this script hasn't seen some of the preceding changes\n"
				"so those changes have been undone and a branch was created.\n\n"
				"You don't have to do anything--other members will merge the branched scripts.\n\n"
				"(You can turn off this message using Program>Options>Script Conflicts.");
		}
	}
	beginnerHelp = !detector->FilesLeftCheckedOut ();	// only display beginner help if no checked out files
	overallMeter.SetRange (0, 10);
	if (!ExecuteSetChange (hdr, cmdList, overallMeter, specificMeter, detector.get ()))
	{
		// Script not executed - mark it as missing
		MarkMissing (hdr.ScriptId ());
		throw Win::InternalException ("Cannot execute corrupted script.\n"
									  "Script will be marked as \"missing\" and a re-send request will be sent.",
									  hdr.GetComment ().c_str ());
	}
	overallMeter.StepAndCheck ();
	if (detector->IsConflict ())
	{
		overallMeter.SetActivity ("Resolving script conflict.");
		overallMeter.StepAndCheck ();
		ResolveConflict (*detector);
	}

	if (isForceAccept)
	{
		overallMeter.SetActivity ("Finishing full sync script processing.");
		overallMeter.StepAndCheck ();
		AcceptSynch (specificMeter);

		overallMeter.SetActivity ("Executing scripts from the full sync.");
		overallMeter.StepAndCheck ();
		AckBox ackBox;
		ExecuteForcedScripts (ackBox, specificMeter);
		Assert (ackBox.empty ());
		// If there any scripts in the in-box unpack them
		ProcessMail ();
		_needRefresh = true;
	}

	SetCheckedOutMarker ();
	SetIncomingScriptsMarker ();
	dbg << "<-- Model::ExecuteSetScript" << std::endl;
	return true;
}

void Model::VerifySender (ScriptHeader const & hdr) const
{
	GlobalIdPack scriptId = hdr.ScriptId ();
	UserId senderId = scriptId.GetUserId ();
	Project::Db const & projectDb = _dataBase.GetProjectDb ();
	MemberState senderState;
	if (projectDb.IsProjectMember (senderId))
	{
		std::unique_ptr<MemberDescription> sender
			= projectDb.RetrieveMemberDescription (senderId);
		// If not in auto-sync mode and sending acknowledgement (we don't send
		// acknowledgement when unpacking tentative scripts from full sync)
		// check sender's state.
		if (!IsAutoSynch ())
		{
			senderState = projectDb.GetMemberState (senderId);
			if (senderState.IsDead ())
			{
				if (!senderState.HasDefected ())
				{
					// Warn about script from removed project member
					MemberNameTag tag (sender->GetName (), senderId);
					std::string info ("You are unpacking script from the user ");
					info += tag;
					info += "\n";
					info += "who has been removed from the project by the administrator.\n";
					info += "Ask this user to defect.";
					TheOutput.Display (info.c_str ());
				}
			}
			else if (senderState.IsObserver () && _dataBase.IsProjectAdmin ())
			{
				// This user is project Administrator -- warn about script from observer
				MemberNameTag tag (sender->GetName (), senderId);
				std::string info ("You are unpacking script from the project observer ");
				info += tag;
				info += "\n";
				info += "You have not received membership update changing this user status to 'voting member'.";
				TheOutput.Display (info.c_str ());
			}
			else if (!_userPermissions.IsTrial ())
			{
				// Check sender license before accepting this script
				sender->VerifyLicense ();
			}
		}
	}
}

void Model::MarkMissing (GlobalId scriptId)
{
	Transaction xact (*this, _pathFinder);
	_history.XMarkMissing (scriptId);
	xact.Commit ();
}

void Model::ResolveConflict (ConflictDetector const & detector)
{
	Assert (detector.IsConflict ());
	// Resolve script conflict
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);
	for (ConflictDetector::FileIterator iter (detector); !iter.AtEnd (); iter.Advance ())
	{
		GlobalId gid = iter.GetGlobalId ();
		FileData const * fd = _dataBase.XGetFileDataByGid (gid);
		FileState state = fd->GetState ();
		if (iter.IsToBeLeftCheckedOut ())
		{
			// Check if file after script execution is still in the project
			if ((state.IsPresentIn (Area::Project) || state.IsPresentIn (Area::Synch)) &&
				state.IsRelevantIn (Area::Original))
			{
				// Restored file/folder is still in the project and checked out
				_checkInArea.Notify (changeAdd, gid);
				_directory.Notify (changeEdit, gid);
				if (!state.IsMerge ())
				{
					Transformer trans (_dataBase, gid);
					trans.AcceptSynch (_pathFinder, fileList, _synchArea);
					_synchArea.Notify (changeRemove, gid);
				}
				continue;
			}
		}

		// file is either not to be left checked out or
		// file is to be left checked out and is not relevant in original or
		// file is to be left checked out but is deleted in project and synch areas
		Assert (!iter.IsToBeLeftCheckedOut ()
			|| iter.IsToBeLeftCheckedOut () 
				&& (!state.IsPresentIn (Area::Project) && !state.IsPresentIn (Area::Synch)) 
					|| !state.IsRelevantIn (Area::Original));

		if (iter.IsToBeLeftCheckedOut () || detector.IsAutoMerge ())
		{
			_checkInArea.Notify (changeAdd, gid);
			_synchArea.Notify (changeAdd, gid);
		}
		else
		{
			Assert (!iter.IsToBeLeftCheckedOut () && !detector.IsAutoMerge ());
			// Forget edits in the restored file/folder.
			Transformer trans (_dataBase, gid);
			trans.Uncheckout (_pathFinder, fileList);
			trans.AcceptSynch (_pathFinder, fileList, _synchArea);
			_synchArea.Notify (changeRemove, gid);
			_checkInArea.Notify (changeRemove, gid);
		}
		_directory.Notify (changeEdit, gid);
	}
	xact.Commit ();
}

void Model::XCopy2Synch(Workspace::HistorySelection & conflictSelection,
						TransactionFileList & fileList,
						Progress::Meter & meter)
{
	meter.SetRange (0, conflictSelection.size (), 1);
	meter.SetActivity ("Copying restored files.");
	for (Workspace::Selection::Sequencer seq (conflictSelection); !seq.AtEnd (); seq.Advance ())
	{
		meter.StepAndCheck ();
		GlobalId gid = seq.GetItem ().GetItemGid ();
		Transformer trans (_dataBase, gid);
		trans.CopyReference2Synch (_pathFinder, fileList);
		XPhysicalFile restoredFile (gid, _pathFinder, fileList);
		CheckSum checkSum = trans.GetCheckSum (Area::Synch, restoredFile);
		// Note: Synch Area contains restored file checksum
		// which is different from the file checksum stored in the database.
		FileData const * fd = _dataBase.XGetFileDataByGid (gid);
		_synchArea.XAddSynchFile (*fd, checkSum);
	}
}

void Model::XPrepareSynch (Workspace::ScriptSelection & scriptSelection, GlobalId scriptId, bool isFromSelf)
{
	for (Workspace::Selection::Sequencer seq (scriptSelection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Item const & item = seq.GetItem ();
		GlobalId itemGid = item.GetItemGid ();
		if (isFromSelf && !item.HasEffectiveSource () && item.HasEffectiveTarget ())
		{
			// If unpacking our own script and script adds new files to the project
			// update database file counter.
			Assert (item.GetOperation () == Workspace::Synch);
			_dataBase.XUpdateFileCounter (itemGid);
		}
		if (item.GetOperation () == Workspace::Synch)
		{
			// Script operation
			Workspace::ScriptItem const & scriptItem = seq.GetScriptItem ();
			FileCmd const & fileCmd = scriptItem.GetFileCmd ();
			if (scriptItem.IsRestored ())
			{
				// Restored files are already present in the Synch Area.
				// Update checksum in the Synch Area
				_synchArea.XUpdateCheckSum (itemGid, fileCmd.GetNewCheckSum ());
			}
			else
			{
				// Add new file to the Synch Area
				_synchArea.XAddSynchFile (fileCmd); 
			}
		}
		// Make sure that all script items not checked out are removed from the check-in area
		FileData const * fd = _dataBase.XGetFileDataByGid (itemGid);
		FileState state = fd->GetState ();
		if (!state.IsRelevantIn (Area::Original))
			_checkInArea.Notify (changeRemove, itemGid);
	}
}

void Model::XMerge (TransactionFileList & fileList, Progress::Meter & meter)
{
	meter.SetRange (0, _synchArea.XCount () + 1);
	meter.SetActivity ("Updating project files.");
	for (XSynchAreaSeq seq (_synchArea); !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		char const * tgtPath = _pathFinder.XGetRootRelativePath (gid);
		meter.SetActivity (tgtPath);
		meter.StepAndCheck ();
		Transformer trans (_dataBase, gid);
		FileState preMergeState = trans.GetState ();
		trans.Merge (_pathFinder, fileList, _synchArea, meter);
		FileState postMergeState = trans.GetState ();
		if (preMergeState.IsRelevantIn (Area::Original))
		{
			if (postMergeState.IsRelevantIn (Area::Original))
			{
				// After sync item is still checked out
				_checkInArea.Notify (changeEdit, gid);
				_directory.Notify (changeEdit, gid);
			}
			else
			{
				// After sync item is no longer checked out
				_checkInArea.Notify (changeRemove, gid);
				if (preMergeState.IsCoDelete ())
				{
					// Item was locally deleted
					if (postMergeState.IsSoDelete ())
					{
						// Synch also deletes item -- no need to update file view
					}
					else
					{
						// Synch removes item -- bring it back to the file view
						_directory.Notify (changeAdd, gid);
					}
				}
				else
				{
					// Item was locally removed -- can be refreshed only by name, because its global id is set to invalid
					UniqueName const & uname = trans.GetUniqueName ();
					if (postMergeState.IsSoDelete ())
					{
						// Synch deletes item -- remove it from the file view
						_directory.Notify (changeRemove, uname.GetName ().c_str ());
					}
					else
					{
						// Synch also removes item
						_directory.Notify (changeEdit, uname.GetName ().c_str ());
					}
				}
			}
		}
		else
		{
			// If file was not checked out before the merge it cannot be checked out after the merge.
			// If folder was not checked out before the merge it can be checked out after the merge
			// when the sync is deleteing/removing the folder from the project and locally the user
			// has checked out files in the deleted/removed folder -- merge will checkout the folder.
			Assert (!postMergeState.IsRelevantIn (Area::Original) || trans.IsFolder ());
			_directory.Notify (changeEdit, gid);
		}
	}
}

void Model::UpdateAssociationList (Workspace::ScriptSelection const & scriptSelection)
{
	AssociationList associationList (_catalog);
	for (Workspace::Selection::Sequencer seq (scriptSelection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::ScriptItem const & item = seq.GetScriptItem ();
		FileCmd const & fileCmd = item.GetFileCmd ();
		SynchKind synchKind = fileCmd.GetSynchKind ();
		if (synchKind == synchNew)
		{
			FileType type = item.GetEffectiveTargetType ();
			if (type.IsFolder ())
				continue;

			// New file added to the project -- check if we have file type association for its extension
			std::string const & targetName = item.GetEffectiveTargetName ();
			PathSplitter splitter (targetName);
			FileType fileType = associationList.GetFileType (splitter.GetExtension ());
			if (fileType.IsInvalid ())
				associationList.Add (splitter.GetExtension (), type);
		}
	}
}

void Model::CollectDeletedFolders (Workspace::ScriptSelection const & scriptSelection,
								   GidList & deletedFolders)
{
	for (Workspace::Selection::Sequencer seq (scriptSelection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::ScriptItem const & item = seq.GetScriptItem ();
		FileCmd const & fileCmd = item.GetFileCmd ();
		SynchKind synchKind = fileCmd.GetSynchKind ();
		if (synchKind == synchDelete)
		{
			FileType type = item.GetEffectiveSourceType ();
			if (!type.IsFolder ())
				continue;	// Skip files
			if (item.GetSourceParentItem () != 0)
				continue;	// Skip folder whose parent folder is included in the script selection

			deletedFolders.push_back (item.GetItemGid ());
		}
	}
}

// Returns true if file script executed successfully
bool Model::ExecuteSetChange (ScriptHeader const & hdr,
							  CommandList const & cmdList,
							  Progress::Meter & overallMeter,
							  Progress::Meter & specificMeter,
							  ConflictDetector const * detector)
{
	dbg << "--> Model::ExecuteSetChange" << std::endl;
	GlobalIdPack scriptId = hdr.ScriptId ();
	UserId senderId = scriptId.GetUserId ();
	bool isFromSelf = false;
	bool changeSenderStateToObserver = false;
	bool forcedExecution = (detector == 0);
	bool isScriptConflict = forcedExecution ? false : detector->IsConflict ();

	if (!forcedExecution)
	{
		MemberState senderState;
		Project::Db const & projectDb = _dataBase.GetProjectDb ();
		isFromSelf = (senderId == projectDb.GetMyId ());
		if (projectDb.IsProjectMember (senderId))
			senderState = projectDb.GetMemberState (senderId);
		changeSenderStateToObserver = senderState.IsObserver () && projectDb.IsProjectAdmin ();
	}

	Workspace::ScriptSelection scriptSelection (cmdList);
	dbg << "Model::ExecuteChangeScript -- initial selection" << std::endl;
	dbg << scriptSelection;
	// Update file type association list for added files
	UpdateAssociationList (scriptSelection);
	GidList deletedFolders;
	CollectDeletedFolders (scriptSelection, deletedFolders);

	if (!forcedExecution)
	{
		scriptSelection.Extend (_dataBase);
		dbg << "Model::ExecuteChangeScript -- extended selection" << std::endl;
		dbg << scriptSelection;
	}

	AckBox ackBox;
	TransactionFileList fileList;

	{
		// Synch transaction scope
		SynchTransaction xact (*this, _pathFinder, _dataBase, fileList);

		if (!scriptSelection.empty () || isScriptConflict)
			_synchArea.XAddScriptFile (hdr.GetComment (), scriptId);

		if (isScriptConflict)
		{
			Assert (!forcedExecution);
			// Reverse local changes to recreate reference versions used by the received script
			Workspace::HistorySelection & conflictSelection = detector->GetSelection ();
			dbg << "    Undoing changes from the rejected scripts" << std::endl;
			overallMeter.SetActivity ("Undoing changes from the rejected scripts.");
			overallMeter.StepAndCheck ();
			XExecute (conflictSelection, fileList, specificMeter);
			scriptSelection.MarkRestored (conflictSelection);
			// Copy all restored files to sync area
			dbg << "    Copy restored files to the Synch Area" << std::endl;
			XCopy2Synch (conflictSelection, fileList, specificMeter);
			// Mark rejected scripts
			dbg << "    Marking scripts as rejected" << std::endl;
			for (ConflictDetector::ScriptIterator iter (*detector); !iter.AtEnd (); iter.Advance ())
			{
				GlobalId scriptId = iter.GetScriptId ();
				_history.XMarkUndone (scriptId);
			}
		}

		// Execute commands from received script
		if (!forcedExecution)
		{
			Workspace::XGroup workgroup (_dataBase, scriptSelection);
			scriptSelection.XMerge (workgroup);
		}
		dbg << "    Execute Commands From Received Script" << std::endl;
		overallMeter.SetActivity ("Executing commands from the incoming script.");
		overallMeter.StepAndCheck ();
		try
		{
			XExecute (scriptSelection, fileList, specificMeter);
		}
		catch (ScriptCorruptException)
		{
			return false;
		}

		XPrepareSynch (scriptSelection, scriptId, isFromSelf);

		// For all files in Synch Area perform merge
		dbg << "    Merge Files From Synch Area" << std::endl;
		overallMeter.SetActivity ("Preparing project files update.");
		overallMeter.StepAndCheck ();
		XMerge (fileList, specificMeter);

		// Remember script in the project history
		overallMeter.SetActivity ("Storing received script in the history and sending acknowledgment script.");
		overallMeter.StepAndCheck ();
		if (isFromSelf)
		{
			// We just unpacked our own script which doesn't
			// have to be made reference version. This can happen when
			// our local project database has been restored from
			// some file backup and we are processing re-send scripts.
			// We have to update our script counter, so we don't use
			// already taken script ids.
			_dataBase.XUpdateScriptCounter (scriptId);
			// Revisit: maybe just in case we should re-broadcast make reference version script ?
		}
		_history.XMarkExecuted (hdr);

		if (changeSenderStateToObserver)
		{
			// Project administrator knows the sender of this sync script
			// as project observer.  The sender must have changed its state
			// to voting member, but the administrator doesn't know about this.
			// We have accepted this script, but we will broadcast
			// membership update script changing sender state to observer
			StateObserver observer (_dataBase.XGetMemberState (senderId));
			XCheckinStateChange (senderId, observer, ackBox);
		}

		xact.Commit ();
	}

	if (!forcedExecution && detector->IsAutoMerge ())
	{
		overallMeter.SetActivity ("Performing automatic merge of script changes.");
		overallMeter.StepAndCheck ();
		// Automatically merge content if necessary
		dbg << "    Automatic merge of script changes" << std::endl;
		for (SynchAreaSeq seq (_synchArea); !seq.AtEnd (); seq.Advance ())
		{
			GlobalId gid = seq.GetGlobalId ();
			FileData const * fd = _dataBase.GetFileDataByGid (gid);
			FileState state = fd->GetState ();
			if (state.IsMergeContent ())
			{
				PhysicalFile file (*fd, _pathFinder);
				MergeContent (file, specificMeter);
			}
		}
	}

	PostDeleteFolder (deletedFolders);
	SendAcknowledgments (ackBox);
	dbg << "<-- Model::ExecuteSetChange" << std::endl;
	return true;
}

void Model::ExecuteMembershipChange (ScriptHeader const & hdr,
									 CommandList const & cmdList,
									 AckBox & ackBox)
{
	MembershipUpdateVerifier verifier (hdr, cmdList, IsQuickVisit ());
	verifier.CheckUpdate (_dataBase.GetProjectDb (), _history, _userPermissions);

	Transaction xact (*this, _pathFinder);
	//--------------------------------------------------------

	if (verifier.CanExecuteMembershipUpdate ())
	{
		Project::Db & projectDb = _dataBase.XGetProjectDb ();
		CommandList::Sequencer seq (cmdList);
		MemberCmd const & memberCmd = seq.GetMemberCmd ();
		std::unique_ptr<CmdMemberExec> exec = memberCmd.CreateExec (projectDb);
		ThisUserAgent agent (_userPermissions);
		exec->Do (agent);
		if (!projectDb.XIsProjectMember (memberCmd.GetUserId ()))
		{
			// After executing membership update member is NOT recorded in the project database -- very strange!
			std::string missingId ("Missing member id: ");
			missingId += ToHexString (memberCmd.GetUserId ());
			throw Win::Exception ("Illegal membership update: edited project member not present in the project database", missingId.c_str ());
		}
		agent.XFinalize (projectDb, _catalog, _pathFinder, _project.GetCurProjectId ());
		if (!hdr.IsAddMember ())
		{
			MemberState state = projectDb.XGetMemberState (memberCmd.GetUserId ());
			if (state.IsDead () || state.IsObserver ())
				_history.XRemoveFromAckList (memberCmd.GetUserId (), state.IsDead (), ackBox);
			if (state.IsDead ())
			{
				_history.XCleanupMemberTree (memberCmd.GetUserId ());
				GidList emptyList;
				_checkedOutFiles.XUpdate(emptyList, memberCmd.GetUserId ());
			}
		}
	}
	_history.XMarkExecuted (hdr);

	if (verifier.HasResponseUpdate ())
	{
		MemberInfo const & responseUpdate = verifier.GetResponse ();
		XCheckinMembershipChange (responseUpdate, ackBox);
	}

	if (verifier.ConfirmConversion ())
	{
		CommandList::Sequencer seq (cmdList);
		NewMemberCmd const & addMemberCmd = seq.GetAddMemberCmd ();
		MemberInfo const & info = addMemberCmd.GetMemberInfo ();
		Assert (info.State ().IsVerified ());
		UserId thisUserId = _dataBase.XGetMyId ();
		MemberState thisUserState = _dataBase.XGetMemberState (thisUserId);
		if (info.State ().IsAdmin () && thisUserState.IsAdmin ())
		{
			// Converting member claims to be the project administrator and this project member
			// also claims to be the project administrator. This project member has to give up
			// its administrative privilege.
			StateVotingMember voting (thisUserState);
			XCheckinStateChange (thisUserId, voting, ackBox);
		}
		// Confirm conversion to version 4.5 by sending back our own membership update history -- our all membership scripts.
		// Don't include side lineages in the mini full sync package, because recipient
		// may not be able correctly process them -- he/she just converted to version 4.5,
		// so his/hers history is missing all the membership change scripts for other converted
		// project members, that are recorded in our history.  He/she will receive those missing
		// membership script in the mini full sync packages from other converted project members.
		MemberNameTag tag (info.Description ().GetName (), info.Id ());
		std::string action ("receives conversion confirmation from ");
		std::unique_ptr<MemberDescription> thisUserDesc = _dataBase.XRetrieveMemberDescription (_dataBase.XGetMyId ());
		MemberNameTag thisUserTag (thisUserDesc->GetName (), _dataBase.XGetMyId ());
		action += thisUserTag;
		MembershipUpdateComment comment (tag, action);
		ScriptHeader packageHdr (ScriptKindPackage (), gidInvalid, _dataBase.XProjectName ());
		packageHdr.SetScriptId (_dataBase.XMakeScriptId ());
		packageHdr.AddComment (comment);
		ScriptList scriptList;
		_history.XRetrieveThisUserMembershipUpdateHistory (scriptList);
		Lineage packageMainLineage;
		for (ScriptList::Sequencer seq (scriptList); !seq.AtEnd (); seq.Advance ())
		{
			ScriptHeader const & hdr = seq.GetHeader ();
			packageMainLineage.PushId (hdr.ScriptId ());
		}
		packageHdr.AddMainLineage (packageMainLineage);
		GlobalIdPack scriptId (hdr.ScriptId ());
		std::unique_ptr<MemberDescription> confirmationRecipient = _dataBase.XRetrieveMemberDescription (scriptId.GetUserId ());
		std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();
		_mailer.XUnicast (packageHdr, scriptList, *confirmationRecipient, notification.get());
	}

	//--------------------------------------------------------
	xact.Commit ();
}

void Model::ExecuteForcedScripts (AckBox & ackBox, Progress::Meter & meter)
{
	ScriptHeader hdr;
	CommandList cmdList;
	while (_history.RetrieveForcedScript (hdr, cmdList))
	{
		// There are unpacked scripts marked for forced execution.
		// These can be change scripts from the full sync or
		// membership updates.
		if (hdr.IsSetChange ())
		{
			Progress::Meter dummyOverallMeter;
			if (ExecuteSetChange (hdr, cmdList, dummyOverallMeter, meter, 0)) // Conflict detector not needed
			{
				AcceptSynch (meter);
			}
			else
			{
				// Script not executed - mark it as missing
				MarkMissing (hdr.ScriptId ());
				throw Win::InternalException ("Cannot execute corrupted script.\n"
											  "Script will be marked as \"missing\" and a re-send request will be sent.",
											  hdr.GetComment ().c_str ());
			}
		}
		else
		{
			Assert (hdr.IsMembershipChange ());
			ExecuteMembershipChange (hdr, cmdList, ackBox);
			if (hdr.IsDefectOrRemove ())
			{
				// Check if membership update removed this user from the project
				MemberState state = _dataBase.GetMemberState (_dataBase.GetMyId ());
				if (state.IsDead ())
				{
					SendAcknowledgments (ackBox);
					// We remove the project information only from the registry.
					// All project files remain intact.
					int localProjectId = _project.GetCurProjectId ();
					_catalog.ForgetProject (localProjectId);
					BroadcastProjectChange (localProjectId);
					if (!IsQuickVisit ())
					{
						std::string info ("Administrator removed you from the project ");
						info += _dataBase.ProjectName ();
						info += '.';
						TheOutput.Display (info.c_str ());
					}
					// Under transaction delete project database
					{
						Transaction xact (*this, _pathFinder);
						_dataBase.XDefect ();
						_history.XDefect ();
						xact.CommitAndDelete (_pathFinder);
					}
					LeaveProject (true);	// Remove project lock file
					return;	// Don't process any other forced scripts in this project
				}
			}
		}
		hdr.Clear ();
		cmdList.clear ();
	}
}

void Model::Revert (Progress::Meter & overallMeter, Progress::Meter & specificMeter)
{
	SimpleMeter simpleMeter (_uiFeedback);
	std::unique_ptr<Workspace::HistorySelection> selection =
		_historicalFiles.GetRevertSelection (simpleMeter);
	if (selection->empty ())
		return;
	DoRevert (*selection, overallMeter, specificMeter);
}

void Model::Revert (GidList const & files,
					Progress::Meter & overallMeter,
					Progress::Meter & specificMeter)
{
	SimpleMeter simpleMeter (_uiFeedback);
	std::unique_ptr<Workspace::HistorySelection> selection =
		_historicalFiles.GetRevertSelection (files, simpleMeter);
	Assert(!selection->empty ());
	DoRevert (*selection, overallMeter, specificMeter);
}

void Model::DoRevert (Workspace::HistorySelection & selection,
					  Progress::Meter & overallMeter,
					  Progress::Meter & specificMeter)
{
	dbg << "Model::Revert -- revert selection" << std::endl;
	dbg << selection;
	selection.Extend (_dataBase);
	dbg << "Model::Revert -- extended revert selection" << std::endl;
	dbg << selection;

	if (!IsQuickVisit ())
	{
		GidList checkedOut;
		_dataBase.ListCheckedOutFiles (checkedOut);
		if (!checkedOut.empty ())
		{
			for (GidList::const_iterator iter = checkedOut.begin (); iter != checkedOut.end (); ++iter)
			{
				GlobalId gid = *iter;
				if (selection.IsIncluded (gid))
				{
					Out::Answer userChoice = 
						TheOutput.Prompt ("Some of the files to be restored are checked out.\n"
						"If you proceed with the Restore, current edit changes will be lost.\n\n"
						"Are you sure you want to continue?",
						Out::PromptStyle (Out::YesNo, Out::No, Out::Warning),
						TheAppInfo.GetWindow ());
					if (userChoice == Out::No)
						return;

					break;	// Continue reverting files
				}
			}
		}
	}

	overallMeter.SetRange (0, 4);

	//--------------------------------------------------------------------------
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);

	Workspace::XGroup workgroup (_dataBase, selection);
	selection.XMerge (workgroup);
	overallMeter.SetActivity ("Reverting project changes.");
	overallMeter.StepAndCheck ();
	XExecute (selection, fileList, specificMeter);
	overallMeter.SetActivity ("Updating project files.");
	overallMeter.StepAndCheck ();
	specificMeter.SetRange (0, selection.size () + 1);
	for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Item const & item = seq.GetItem ();
		GlobalId gid = item.GetItemGid ();
		specificMeter.SetActivity (_pathFinder.XGetRootRelativePath (gid));
		specificMeter.StepAndCheck ();
		// Move restored files to the project area
		Transformer trans (_dataBase, gid);
		trans.MoveReference2Project (_pathFinder, fileList);
		_checkInArea.Notify (changeAdd, gid);
	}

	overallMeter.SetActivity ("Updating project database.");
	overallMeter.StepAndCheck ();
	xact.Commit ();
	//--------------------------------------------------------------------------

	SetCheckedOutMarker ();
	BroadcastProjectChange ();
}

void Model::XPrepareRevert (Restorer & restorer, TransactionFileList & fileList)
{
	FileData const * fd = _dataBase.XFindByGid (restorer.GetFileGid ());
	if (fd != 0)
	{
		// File recorded in the project database
		History::Path const & basePath = restorer.GetBasePath ();
		if (basePath.IsEmpty ())
		{
			// No changes in the base path - store file current project
			// version in the base area if necessary.
			FileState state = fd->GetState ();
			if (!state.IsNone ())
			{
				// File present in the project
				TmpProjectArea & baseArea = restorer.GetBaseArea ();
				Area::Location areaId;
				if (state.IsCheckedIn ())
					areaId = Area::Project;
				else if (restorer.IsCurrentVersionSelected ())
					areaId = Area::Project;
				else
					areaId = Area::Original;

				if (state.IsPresentIn (areaId))
					baseArea.FileCopy (fd->GetGlobalId (), areaId, _pathFinder);
			}
		}
		Transformer trans (_dataBase, restorer.GetFileGid ());
		trans.CheckOutIfNecessary (_pathFinder, fileList);
		trans.CreateRelevantReferenceIfNecessary (_pathFinder, fileList);
	}
	else
	{
		// File is to be created in the future (it's a virtual "restore")
		FileData const & fd = restorer.GetEarlierFileData ();
		_dataBase.XAddForeignFile (fd);
	}
}

void Model::XRevertPath (History::Path const & path,
						 TmpProjectArea & area,
						 TransactionFileList & fileList)
{
	if (path.IsEmpty ())
		return;

	SimpleMeter meter (_uiFeedback);
	for (History::Path::Sequencer seq (path); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Selection & selection = seq.GetSegment ();
		XExecute (selection, fileList, meter, true);	// Virtual selection execution
	}
	// Store in the temporary area the cumulative result of path execution
	Workspace::Selection const & selection = path.GetLastSegment ();
	for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Item const & item = seq.GetItem ();
		Assert (!item.IsFolder ());
		GlobalId gid = item.GetItemGid ();
		// Copy restored file from reference area to temporary area
		if (item.HasEffectiveTarget ())
			area.FileCopy (gid, Area::Reference, _pathFinder);	// After restore item still in project
	}
}

#if !defined (NDEBUG)
//
// Revert script changes
// Precondition: There is no sync pending!
//
void Model::UndoScript (Workspace::HistorySelection & selection, GlobalId scriptId, MemoryLog & log)
{
	log << "Model::UndoScript -- revert selection" << std::endl;
	log << selection;
	// Extend script selection
	selection.Extend (_dataBase);
	log << "Model::UndoScript -- extended revert selection" << std::endl;
	log << selection;

	//--------------------------------------------------------------------------
	TransactionFileList fileList;
	FileTransaction xact (*this, _pathFinder, fileList);

	Workspace::XGroup workgroup (_dataBase, selection);
	selection.XMerge (workgroup);
	log << "Model::UndoScript -- merged revert selection" << std::endl;
	log << selection;
	Progress::Meter meter;
	XExecute (selection, fileList, meter);
	for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
	{
		Workspace::Item const & item = seq.GetItem ();
		GlobalId gid = item.GetItemGid ();
		// Move restored files to the project area
		Transformer trans (_dataBase, gid);
		trans.MoveReference2Project (_pathFinder, fileList);
		FileState state = trans.GetState ();
		if (state.IsPresentIn (Area::Project))
			trans.MakeCheckedIn (_pathFinder, fileList, Area::Reference);	// Get checksum from the Reference
		if (state.IsNone () || state.IsNew ())
			_directory.Notify (changeAll, trans.GetName ());
		else
			_directory.Notify (changeEdit, gid);
	}

	_history.XMarkUndoneScript (scriptId);

	xact.Commit ();
	//--------------------------------------------------------------------------

}
#endif

//-----------
// Full Sync
//-----------

// Returns true if join request can be accepted
// Initializes context
bool Model::CanAcceptJoinRequest (JoinContext & context)
{
	bool canPromptAndDisplay = true;
	if (IsQuickVisit ())
		canPromptAndDisplay = false;
	else if (_userPermissions.IsDistributor ())
		canPromptAndDisplay = true;
	else
		canPromptAndDisplay = !IsAutoJoin ();
	bool accept = true;

	if (context.Init (_mailBox)) // decodes the script
	{
		if (canPromptAndDisplay)
		{
			if (!_userPermissions.IsTrial () && !GetUserPermissions ().IsDistributor ())
			{
				MemberDescription const & joineeDescription = context.GetJoineeDescription ();
				joineeDescription.VerifyLicense ();	// Nag about un-licensed users only after trial period
			}
			// Ask the admin if he/she wants to accept the join request
			accept = context.WillAccept (_dataBase.ProjectName ());
		}
	}
	else
	{
		context.DeleteScript (true); // init failed
		accept = false;
	}

	if (!accept)
	{
		if (context.ShouldDeleteScript ())
		{
			GlobalId scriptId = context.GetScriptId ();
			Transaction xact (*this, _pathFinder);
			//--------------------------------------------------------------------------
			_mailBox.XDeleteScript (scriptId);
			//--------------------------------------------------------------------------
			xact.Commit ();
			SetIncomingScriptsMarker ();
		}
		return false;
	}

	MemberDescription const & joineeDescription = context.GetJoineeDescription ();
	MemberState joinState = context.GetJoineeInfo ().State ();
	if (joineeDescription.IsLicensed () && joinState.IsVoting () && !joinState.IsReceiver ())
	{
		// Licensed user -- wants to join project as voting member
		Project::Db const & projectDb = _dataBase.GetProjectDb ();
		Project::Seats seats (projectDb, joineeDescription.GetLicense ());
		if (!seats.IsEnoughLicenses ())
		{
			joinState.MakeObserver ();
			context.SetJoineeState (joinState);
			if (canPromptAndDisplay)
			{
				std::string info;
				info += "User ";
				info += joineeDescription.GetName ();
				info += " requested to join the project "; 
				info += projectDb.ProjectName ();
				info += " as a voting member.";
				info += "\n\nWe cannot satisfy this request, because the project "; 
				info += projectDb.ProjectName ();
				info += " doesn't have enough licensed seats.";
				info += "\n\nUser "; 
				info += joineeDescription.GetName (); 
				info += " will be accepted in this project as observer.";
				TheOutput.Display (info.c_str ());
			}
		}
	}
	context.DeleteScript (true);	// Delete join request script after processing
	return true;
}

void Model::ExecuteJoinRequest (ProjectChecker & projectChecker, Progress::Meter & overallMeter, Progress::Meter & specificMeter)
{
	Assert (!IsQuickVisit () || IsAutoJoin ());
	Assert (_dataBase.IsProjectAdmin ());
	Assert (_mailBox.HasExecutableJoinRequest ());

	JoinContext context (_catalog, _activityLog, _userPermissions.GetState ());
	if (!CanAcceptJoinRequest (context)) // initializes context
		return;

	overallMeter.SetActivity("Verifying project");
	overallMeter.StepAndCheck();
	specificMeter.SetActivity("");

	projectChecker.Verify (overallMeter, specificMeter);
	if (RepairProject (projectChecker, "Join request"))
	{
		overallMeter.SetActivity("Creating Full Sync script");
		overallMeter.StepAndCheck();
		AcceptJoinRequest (context);
	}
}

bool Model::RepairProject (ProjectChecker & projectChecker, char const * task) const
{
	if (projectChecker.IsFileRepairNeeded ())
	{
		if (!projectChecker.Repair ())
		{
			if (!IsQuickVisit ())
			{
				std::string info = task;
				info += " cannot be serviced until project is successfully repaired.";
				TheOutput.Display (info.c_str ());
			}
			return false;
		}
	}
	return true;
}

void Model::AcceptJoinRequest (JoinContext & context)
{
	FullSynchData fullSynchData;
	ScriptList & setScriptList = fullSynchData.GetSetScriptList ();
	PrepareSetScriptList (setScriptList);
	fullSynchData.InitLineageFromScripts ();

	GidSet skipMembers;
	if (context.GetJoineeInfo ().State ().IsReceiver ())
	{
		_dataBase.GetProjectDb ().GetReceivers (skipMembers);
	}
	GidList allMembers;
	_dataBase.GetProjectDb ().GetHistoricalMemberList (allMembers, skipMembers);

	auto_vector<UnitLineage> & sideLineages = fullSynchData.GetSideLineages ();
	_history.GetMembershipScriptIds (allMembers, sideLineages); // without the "add joinee" script ID
	ScriptList & membershipScriptList = fullSynchData.GetMembershipScriptList ();
	// Add project members with normalized script comments
	_history.GetMembershipScripts (membershipScriptList, sideLineages.begin (), sideLineages.end ());
	ScriptList::EditSequencer membScriptSeq (membershipScriptList);
	_history.PatchMembershipScripts (allMembers,
									membScriptSeq, 
									fullSynchData.GetMainLineage ().GetReferenceId ());
	AckBox ackBox;

	{
		// Transaction scope
		Transaction xact (*this, _pathFinder);
		//--------------------------------------------------------------------------
		XCreateFullSynch (fullSynchData, context, ackBox);
		if (!context.IsInvitation ())
		{
			GlobalId scriptId = context.GetScriptId ();
			_mailBox.XDeleteScript (scriptId);
		}
		//--------------------------------------------------------------------------
		xact.Commit ();
	}

	SetIncomingScriptsMarker ();
	SendAcknowledgments (ackBox);
	dbg << "end full sync" << std::endl;
}

void Model::FullSynchResend ()
{
	dbg << "---->	Creating full sync resend" << std::endl;

	FullSynchData fullSynchData;
	std::vector<unsigned> chunkList;
	unsigned maxChunkSize, chunkCount;
	if (!_sidetrack.GetFullSynchRequest (fullSynchData, chunkList, maxChunkSize, chunkCount))
		return;
	Lineage const & mainLineage = fullSynchData.GetMainLineage ();
	Lineage::Sequencer linSeq (mainLineage);
	Assert (!linSeq.AtEnd ());
	GlobalId inventoryId = linSeq.GetScriptId ();
	ScriptList & scriptList = fullSynchData.GetSetScriptList ();
	CreateFileInventory (inventoryId, scriptList);

	linSeq.Advance ();

	while (!linSeq.AtEnd ())
	{
		std::unique_ptr<ScriptHeader> hdr (new ScriptHeader);
		std::unique_ptr<CommandList> cmdList (new CommandList);
		_history.RetrieveScript (linSeq.GetScriptId (), *hdr, *cmdList);
		scriptList.push_back (std::move(hdr), std::move(cmdList));
		linSeq.Advance ();
	}

	auto_vector<UnitLineage> & sideLineages = fullSynchData.GetSideLineages ();
	auto_vector<UnitLineage>::const_iterator sideLineagesSeq = sideLineages.begin ();
	ScriptList & membershipScriptList = fullSynchData.GetMembershipScriptList ();
	// Retrieve the script adding joinee to the project database and place it on the membership script list
	// with the original comment (this is how it was done when the join request was serviced)
	UnitLineage const & unitLineage = **sideLineagesSeq;
	Assert (unitLineage.GetUnitType () == Unit::Member);
	Assert (unitLineage.Count () == 1);
	GlobalId userId = unitLineage.GetUnitId ();
	Lineage::Sequencer seq (unitLineage);
	GlobalId scriptId = seq.GetScriptId ();
	std::unique_ptr<ScriptHeader> joineeHdr (new ScriptHeader);
	std::unique_ptr<CommandList> joineeCmdList (new CommandList);
	_history.RetrieveScript (scriptId, *joineeHdr, *joineeCmdList, Unit::Member);
	membershipScriptList.push_back (std::move(joineeHdr), std::move(joineeCmdList));
	++sideLineagesSeq;
	// Add the rest project members with normalized script comments
	_history.GetMembershipScripts (membershipScriptList, sideLineagesSeq, sideLineages.end ());

	ScriptList::EditSequencer membScriptSeq (membershipScriptList);
	// Skip joinee script
	Assert (membScriptSeq.GetCmdListSize () == 1);
	CommandList::EditSequencer const & cmdSeq = membScriptSeq.GetCmdSequencer ();
	NewMemberCmd & addJoineeCmd = cmdSeq.GetAddMemberCmd ();
	membScriptSeq.Advance ();

	// Patch the rest
	GidList allMembers;
	_dataBase.GetProjectDb ().GetHistoricalMemberList (allMembers);
	_history.PatchMembershipScripts (allMembers,
									membScriptSeq, 
									inventoryId);


	ScriptList fullSynchPackage;
	fullSynchPackage.Append (fullSynchData.GetMembershipScriptList ());
	fullSynchPackage.Append (fullSynchData.GetSetScriptList ());
	
	Transaction xact (*this, _pathFinder);
	//--------------------------------------------------------------------------
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	ScriptHeader fullSynchHdr (ScriptKindFullSynch (), gidInvalid, projectDb.XProjectName ());
	fullSynchHdr.SetScriptId (projectDb.XMakeScriptId ());
	fullSynchHdr.AddComment ("Full Synch Resend");
	fullSynchHdr.AddMainLineage (mainLineage);
	fullSynchHdr.SwapSideLineages (sideLineages);
	fullSynchHdr.SetChunkInfo (chunkList [0], chunkCount, maxChunkSize);
	std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();
	if (chunkList.size () == 1)
	{
		_mailer.XUnicast (fullSynchHdr, 
			fullSynchPackage, 
			addJoineeCmd.GetMemberInfo ().Description (),
			notification.get());
	}
	else
	{
		_mailer.XUnicast (chunkList, 
			fullSynchHdr, 
			fullSynchPackage, 
			addJoineeCmd.GetMemberInfo ().Description (),
			notification.get());
	}
	_sidetrack.XRemoveFullSynchChunkRequest (inventoryId);
	//--------------------------------------------------------------------------
	xact.Commit ();
	dbg << "<----	End of creating full sync resend" << std::endl;
}

// setScriptList contains initial inventory and unconfirmed scripts
// fullSynchScriptList contains member scripts except for the "add joinee" script
void Model::XCreateFullSynch (FullSynchData & fullSynchData,
							  JoinContext & context,
							  AckBox & ackBox)
{
	MemberInfo joineeInfo (context.GetJoineeInfo ());
	Assert (joineeInfo.GetPreHistoricScript () == gidInvalid && joineeInfo.GetMostRecentScript () == gidInvalid);
	dbg << "---->	Creating full sync under transaction" << std::endl;
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	Assert (projectDb.XIsProjectAdmin ());
	// New user wants to join the project
	MemberDescription const & joineeDescription = joineeInfo.Description ();
	std::string joineeOldUserId (joineeDescription.GetUserId ()); // <- Random user id
	// Assign new user his/her project member id
	joineeInfo.SetUserId (projectDb.XMakeUserId ());
	joineeInfo.SetPreHistoricScript (GlobalIdPack (joineeInfo.Id (), 0));

	// Check in the script adding the joinee.
	MemberNameTag tag (joineeDescription.GetName (), joineeInfo.Id ());
	MembershipUpdateComment comment (tag, "joins the project");
	GlobalId addJoineeScriptId = projectDb.XMakeScriptId ();
	std::unique_ptr<ScriptHeader> newUserAnnouncementHdr (new ScriptHeader (ScriptKindAddMember (),
																		  joineeInfo.Id (),
																		  projectDb.XProjectName ()));
	newUserAnnouncementHdr->SetScriptId (addJoineeScriptId);
	newUserAnnouncementHdr->AddComment (comment);
	// Note: new receiver announcement is not sent to receivers, so receiver side lineages are OK
	_history.XGetLineages (*newUserAnnouncementHdr, UnitLineage::Minimal, joineeInfo.IsReceiver()); // include side lineages
	std::unique_ptr<CommandList> newUserAnnouncementCmdList (new CommandList);
	std::unique_ptr<ScriptCmd> newMember (new NewMemberCmd (joineeInfo));
	newUserAnnouncementCmdList->push_back (std::move(newMember));
	// Check if we have to broadcast the new user announcement.
	bool broadcastNeeded = projectDb.XScriptNeeded (true);	// isMemberChange

	// Add new user description to the project database
	// Note: we add joining member to the project database AFTER checking if
	// new user announcement broadcast is needed -- joining user should not count.
	projectDb.XAddMember (joineeInfo);

	// Store new user announcement in our history
	// now acknowledgement list will contain the joining member.
	_history.XAddCheckinScript (*newUserAnnouncementHdr, *newUserAnnouncementCmdList, ackBox);

	// Add the script adding the joining user first.
	// This script has to be the first one, so the full sync recipient can change 
	// his/her user id in the project database.
	ScriptList fullSynchPackage;

	// Retrieve the "add joinee" script from history (don't reuse the one we have, it has side lineages)
	std::unique_ptr<ScriptHeader> joineeHdr (new ScriptHeader);
	std::unique_ptr<CommandList> joineeCmd (new CommandList);
	_history.XRetrieveScript (addJoineeScriptId, *joineeHdr, *joineeCmd, Unit::Member);
	dbg << "First script " << std::hex << joineeHdr->ScriptId () << " - "
		<< joineeHdr->GetComment () << std::endl;
	fullSynchPackage.push_back (std::move(joineeHdr), std::move(joineeCmd)); // <-First script
	dbg << "Followed by membership scripts" << std::endl;
	fullSynchPackage.Append (fullSynchData.GetMembershipScriptList ()); // <- Membership scripts

	// insert the joinee announcement before all other sideLineages 
	std::unique_ptr<UnitLineage> joineeLineage (new UnitLineage (Unit::Member, joineeInfo.Id ()));
	joineeLineage->PushId (addJoineeScriptId);
	auto_vector<UnitLineage> & sideLineages = fullSynchData.GetSideLineages ();
	sideLineages.insert (0, std::move(joineeLineage)); // insert at offset 0

	dbg << "Followed by set script list " << std::endl;
	fullSynchPackage.Append (fullSynchData.GetSetScriptList ()); // <- File inventory and tentative scripts

	ScriptHeader fullSynchHdr (ScriptKindFullSynch (), gidInvalid, projectDb.XProjectName ());
	fullSynchHdr.SetScriptId (projectDb.XMakeScriptId ());
	fullSynchHdr.AddComment ("Full Sync");
	Lineage & mainLineage = fullSynchData.GetMainLineage ();
	dbg << "-- Main Lineage -- " << mainLineage << std::endl;
	dbg << "-- Side Lineages -- " << std::endl;
#if !defined NDEBUG
	for (auto_vector<UnitLineage>::const_iterator slit = sideLineages.begin ();
		slit != sideLineages.end (); ++slit)
	{
		dbg << **slit << std::endl;
	}
#endif
	fullSynchHdr.AddMainLineage (mainLineage);
	fullSynchHdr.SwapSideLineages (sideLineages);

	XSendFullSynch (joineeInfo,
					joineeOldUserId,
					fullSynchHdr,
					fullSynchPackage,
					*newUserAnnouncementHdr,
					*newUserAnnouncementCmdList,
					broadcastNeeded, 
					context);

	dbg << "<----	End of creating full sync under transaction" << std::endl;
}

// Send full sync to the New address and attach Dispatcher address
// change section changing join request sender address from old (using
// random userId) to New one using official new user userId.
void Model::XSendFullSynch (MemberInfo const & joineeMemberInfo,
							std::string const & joineeOldUserId, 
							ScriptHeader & fullSynchHdr,
							ScriptList const & fullSynchPackage, 
							ScriptHeader & newUserAnnouncementHdr,
							CommandList const & newUserAnnouncementCmdList, 
							bool broadcastNeeded,
							JoinContext & context)
{
	dbg << "--> Model::XSendFullSynch" << std::endl;
	Assert (joineeMemberInfo.GetMostRecentScript () == gidInvalid);
	Project::Db & projectDb = _dataBase.XGetProjectDb ();
	MemberDescription const & joineeDescription = joineeMemberInfo.Description ();

	// Prepare full sync trailer containing membership commands adding joinee and
	// project administrator to the joining enlistment member database.
	ScriptTrailer trailer;
	std::unique_ptr<ScriptCmd> newMemberCmd (new NewMemberCmd (joineeMemberInfo));
	trailer.push_back (std::move(newMemberCmd));
	std::unique_ptr<MemberInfo> admin = projectDb.XRetrieveMemberInfo (projectDb.XGetMyId ());
	// Note: reseting script markers will guarantee that nothing will be deleted from mailbox.
	// The joining member will start unpacking scripts only after the full sync overwrote these markers.
	admin->ResetScriptMarkers ();
	std::unique_ptr<ScriptCmd> adminCmd (new NewMemberCmd (*admin));
	trailer.push_back (std::move(adminCmd));

	// Create Dispatcher script
	DispatcherScript dispatcherScript (context.IsInvitation ());
	if (context.IsInvitation ())
	{
		Invitee invitee (Address (joineeDescription.GetHubId (), 
								  _dataBase.ProjectName (), 
								  joineeMemberInfo.GetUserId ()), 
								  joineeDescription.GetName (), 
								  context.GetComputerName (), 
								  !joineeMemberInfo.IsVoting ());

		MemberState myState = _dataBase.XGetMemberState (joineeMemberInfo.Id ());
		MemberInfo inviteeDefectInfo (joineeMemberInfo.Id (), 
									  StateDead (myState, true), 
									  joineeDescription);

		XMembershipChange inviteeDefect (inviteeDefectInfo, _dataBase.XGetProjectDb ());
		std::string defectFilename;
		std::vector<unsigned char> defectScript;
		inviteeDefect.BuildFutureDefect (_mailer, _history, defectFilename, defectScript);

		std::unique_ptr<DispatcherCmd> invitationCmd (
			new InvitationCmd (admin->Name (), 
							   admin->HubId (), 
							   invitee, 
							   defectFilename, 
							   defectScript));
		dispatcherScript.AddCmd (std::move(invitationCmd));
	}
	else
	{
		// change New user address
		std::unique_ptr<DispatcherCmd> addressChange (
			new AddressChangeCmd (joineeDescription.GetHubId (),
								  joineeOldUserId,	// Before assigning user id
								  joineeDescription.GetHubId (),
								  joineeDescription.GetUserId ())); // After assigning user id
		dispatcherScript.AddCmd (std::move(addressChange));
	}

	// Add the full sync sender's return transport
	Transport myPublicInboxTransport = _catalog.GetActiveIntraClusterTransportToMe ();
	if (!myPublicInboxTransport.IsUnknown ())
	{
		std::unique_ptr<MemberDescription> thisUser 
			= projectDb.XRetrieveMemberDescription (projectDb.XGetMyId ());
		std::unique_ptr<DispatcherCmd> addMember (new AddMemberCmd (thisUser->GetHubId(),
																  thisUser->GetUserId (),
																  myPublicInboxTransport));
		dispatcherScript.AddCmd (std::move(addMember));
	}

	std::unique_ptr<CheckOut::List> notification = XGetCheckoutNotification ();

	if (context.IsManualInvitationDispatch ())
	{
		Assert (context.IsInvitation ());
		// Transfers the ownership of temporary file
		context.SetInvitationFileName (_mailer.XSaveInTmpFolder (fullSynchHdr, 
																 fullSynchPackage, 
																 joineeDescription, 
																 trailer, 
																 dispatcherScript));
	}
	else
	{
		_mailer.XUnicast (fullSynchHdr, 
						  fullSynchPackage, 
						  joineeDescription,
						  notification.get (),
						  &trailer, 
						  &dispatcherScript);
	}

	// Broadcast new user announcement if necessary
	if (broadcastNeeded)
	{
		// Multicast -- broadcast except New User, administrator (this user), and possibly receivers
		GidSet filterOut;
		// Don't tell receivers about another receiver
		if (joineeMemberInfo.IsReceiver ())
			GetProjectDb ().XGetReceivers (filterOut);
		filterOut.insert (projectDb.XGetMyId ());
		filterOut.insert (joineeMemberInfo.Id ());
		_mailer.XMulticast (newUserAnnouncementHdr,
							newUserAnnouncementCmdList,
							filterOut,
							notification.get ());
	}
	dbg << "<-- Model::XSendFullSynch" << std::endl;
}

bool Model::ForwardJoinRequest (std::string const & scriptPath)
{
	Assert (!_dataBase.IsProjectAdmin ());
	if (_dataBase.GetAdminId () == gidInvalid)
	{
		// Project doesn't have administrator
		if (!IsQuickVisit ())
		{
			std::string info ("The project '");
			info += _dataBase.ProjectName ();
			info += "' doesn't have administrator and join request script cannot be processed.\n";
			GidList projectMembers;
			_dataBase.GetAllMemberList (projectMembers);
			bool observersOnly;
			if (projectMembers.empty ())
			{
				// Just one user
				MemberState thisUserState = _dataBase.GetMemberState (_dataBase.GetMyId ());
				observersOnly = thisUserState.IsObserver ();
			}
			else
			{
				observersOnly = true;
				for (GidList::const_iterator iter = projectMembers.begin ();
					iter != projectMembers.end ();
					++iter)
				{
					MemberState state = _dataBase.GetMemberState (*iter);
					if (state.IsActive ())
					{
						observersOnly = false;
						break;
					}
				}
			}
			if (observersOnly)
			{
				info += "Change your state to voting member and "
						"perform emergency administrator election.";
			}
			else
			{
				info += "Perform emergency administrator election.";
			}
			TheOutput.Display (info.c_str ());
		}
		return false;
	}

	_mailer.ForwardJoinRequest (scriptPath);
	return true;
}

void Model::PrepareSetScriptList (ScriptList & fileScriptList)
{
	Lineage lineage;
	_history.GetCurrentLineage (lineage);
	CreateFileInventory (lineage.GetReferenceId (), fileScriptList);
	// Add all tentative set change scripts from the history
	SimpleMeter meter (_uiFeedback);
	_history.GetTentativeScripts (fileScriptList, meter);
}

void Model::CreateFileInventory (GlobalId referenceId, ScriptList & fileScriptList)
{
	dbg << "File inventory: version " << std::hex << referenceId << std::endl;
	History::Range undoRange;
	CreateUndoRange (referenceId, undoRange);
	std::unique_ptr<CommandList> initialFileInventory (new CommandList);
	// Transaction scope
	{
		TmpProjectArea tmpArea;
		Transaction xact (*this, _pathFinder);
		// Temporarily revert project state to the requested version
		//--------------------------------------------------------------------------
		SimpleMeter simpleMeter (_uiFeedback);
		XPrepareFileDb (undoRange, tmpArea, simpleMeter);
		// Add files from the last confirmed version to the full sync script
		_dataBase.XAddProjectFiles (_pathFinder, *initialFileInventory, tmpArea, simpleMeter);
		//--------------------------------------------------------------------------
		// DO NOT COMMIT! Transactions are to be aborted and project state restored
		tmpArea.Cleanup (_pathFinder);
	}
	std::unique_ptr<ScriptHeader> hdr (new ScriptHeader (ScriptKindSetChange (),
													   gidInvalid,
													   _dataBase.ProjectName ()));
	hdr->SetScriptId (referenceId);
	hdr->AddComment ("Files added by the full sync script");
	fileScriptList.push_back (std::move(hdr), std::move(initialFileInventory));
	dbg << "end file inventory" << std::endl;
}

//
// Helper methods
//

void Model::CreateUndoRange (GlobalId selectedId, History::Range & range) const
{
	if (selectedId == gidInvalid)
	{
		// Undo range is empty
		range.Clear ();
	}
	else
	{
		// Creates a script range from the current version (inclusive)
		// to the selected script id (exclusive)
		GidSet emptyFileFilter;
		_history.CreateRangeFromCurrentVersion (selectedId, emptyFileFilter, range);
		range.RemoveOldestId ();	// Remove selected id from the range
	}
}

void Model::XPrepareFileDb (History::Range const & undoRange, 
							TmpProjectArea & tmpArea,
							Progress::Meter & overallMeter,
							bool includeLocalEdits)
{
	SimpleMeter specificMeter (_uiFeedback);
	overallMeter.SetRange(0, 3);
	overallMeter.SetActivity ("Preparing project files.");
	overallMeter.StepAndCheck ();
	// Uncheckout any edited files and for historical version reconstruct files.
	// Start with reconstruction first, so if there are checked out files that
	// have to be reconstructed they participate only in the reconstruction not
	// both reconstruction and un-checkout.
	TransactionFileList fileList;
	if (undoRange.Size () != 0)
	{
		Workspace::HistoryRangeSelection selection (_history,
													undoRange,
													false,		// Backward selection
													specificMeter);
		// Don't need to extend or merge selection, because this is temporary revert
		GlobalIdPack pack (undoRange.GetOldestId ());
		std::string info ("Temporarily reverting project state to the version ");
		info += pack.ToBracketedString ();
		overallMeter.SetActivity (info.c_str ());
		overallMeter.StepAndCheck ();
		XExecute (selection, fileList, specificMeter, true);	// Virtual selection execution
		overallMeter.SetActivity ("Storing reverted project files in the temporary area.");
		overallMeter.StepAndCheck ();
		specificMeter.SetRange (0, selection.size (), 1);
		for (Workspace::Selection::Sequencer seq (selection); !seq.AtEnd (); seq.Advance ())
		{
			Workspace::Item const & item = seq.GetItem ();
			GlobalId gid = item.GetItemGid ();
			specificMeter.SetActivity (_pathFinder.GetRootRelativePath (gid));
			specificMeter.StepIt ();
			Transformer trans (_dataBase, gid);
			if (trans.GetState ().IsPresentIn (Area::Reference))
			{
				// We cannot use here the regular check-in, because it
				// will not set correct checksum for the restored file(s).
				// Why ? Because in the project area the file(s) remain untouched
				// during the whole revert process, so check-in will produce no diff script cmd.
				// We just flip state bits and set checksum.

				// Get checksum from the Reference area
				trans.MakeCheckedIn (_pathFinder, fileList, Area::Reference);
				if (!trans.IsFolder ())
				{
					// Move restored files from reference area to temporary area
					tmpArea.FileMove (gid, Area::Reference, _pathFinder);
				}
			}
			else
			{
				// Restored file/folder not in the project
				trans.MakeNotInProject (false);	// Not recursive
			}
		}
		// Now every reconstructed file temporarily is in the checked in state
	}

	if (!includeLocalEdits)
	{
		// For every remaining checked-out file perform un-checkout
		// and move file from staging area to temporary area
		GidList checkedOut;
		_dataBase.XListCheckedOutFiles (checkedOut);

		for (GidList::const_iterator gidIter = checkedOut.begin (); 
			gidIter != checkedOut.end (); 
			++gidIter)
		{
			GlobalId gid = *gidIter;
			Transformer trans (_dataBase, gid);
			trans.Uncheckout (_pathFinder, fileList, true); // virtual
			if (trans.GetState ().IsPresentIn (Area::Project) && !trans.IsFolder ())
				tmpArea.FileMove (gid, Area::Staging, _pathFinder);
		}
	}
}
