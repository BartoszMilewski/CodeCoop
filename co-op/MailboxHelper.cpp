//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"
#include "MailboxHelper.h"
#include "ScriptHeader.h"
#include "Mailer.h"
#include "FileList.h"
#include "History.h"
#include "Mailbox.h"
#include "Database.h"
#include "CheckoutNotifications.h"
#include "ScriptIo.h"
#include "CtrlScriptProcessor.h"
#include "CmdExec.h"

using namespace Mailbox;

void Agent::RememberNewFolder (GlobalId gid, UniqueName const & uName)
{
	_newFolders.push_back (std::make_pair (gid, uName));
}

void Agent::XFinalize (Mailbox::Db & mailbox,
					   History::Db & history,
					   DataBase & database,
					   CheckOut::Db & checkedoutFiles,
					   CtrlScriptProcessor & ctrlScriptProcessor,
					   CheckOut::List const * notification)
{
	// Process acknowledgments and re-send requests or forward join requests
	for (std::vector<ScriptInfo>::const_iterator iter = _ctrlScripts.begin (); iter != _ctrlScripts.end (); ++iter)
	{
		ScriptInfo const & scriptInfo = *iter;
		std::string const & path = scriptInfo.GetScriptPath ();
		if (scriptInfo.GetScriptState ().IsJoinRequest ())
		{
			ctrlScriptProcessor.XRememberJoinRequest (path);
		}
		else
		{
			try
			{
				std::unique_ptr<ScriptHeader> hdr;
				std::unique_ptr<CommandList> cmdList;
				mailbox.RetrieveScript (path, hdr, cmdList);
				if (scriptInfo.IsFullSynchResendRequest ())
				{
					if (!ctrlScriptProcessor.XRememberResendRequest (*hdr, *cmdList))
					{
						mailbox.XRememberCorruptedScript (scriptInfo);
						continue;	// Leave corrupted script in the inbox
					}
				}
				else
				{
					ctrlScriptProcessor.XExecuteScript (*hdr, *cmdList, history, _ackBox, notification);
				}
			}
			catch ( ... )
			{
				// Ignore all exceptions during control script processing
				_activityLog.CorruptedScript (scriptInfo);
			}
			DeleteScriptFile (path);
		}
	}

	// Process execute-only scripts
	Project::Db & projectDb = database.XGetProjectDb ();
	for (std::vector<ScriptInfo>::const_iterator iter = _executeOnlyScripts.begin ();
		 iter != _executeOnlyScripts.end ();
		 ++iter)
	{
		ScriptInfo const & scriptInfo = *iter;
		Assert (scriptInfo.GetScriptKind ().IsMergeable ());
		// Membership update script
		std::string const & path = scriptInfo.GetScriptPath ();
		try
		{
			std::unique_ptr<ScriptHeader> hdr;
			std::unique_ptr<CommandList> cmdList;
			mailbox.RetrieveScript (path, hdr, cmdList);
			for (CommandList::Sequencer seq (*cmdList); !seq.AtEnd (); seq.Advance ())
			{
				MemberCmd const & memberCmd = seq.GetMemberCmd ();
				std::unique_ptr<CmdMemberExec> exec = memberCmd.CreateExec (projectDb);
				exec->Do (GetThisUserAgent ());
			}
		}
		catch ( ... )
		{
			// Ignore all exceptions during execute only script processing
			_activityLog.CorruptedScript (scriptInfo);
		}
		DeleteScriptFile (path);
	}

	// Remove from the mailbox scripts depending on known prehistoric scripts
	if (!_prehistoricScripts.empty ())
	{
		GidSet currentPrehistoricIds;
		currentPrehistoricIds.insert (_prehistoricScripts.begin (), _prehistoricScripts.end ());
		do
		{
			GidSet newPrehistoricIds;
			for (Mailbox::Db::Sequencer seq (mailbox); !seq.AtEnd (); seq.Advance ())
			{
				ScriptInfo const & info = seq.GetScriptInfo ();
				GlobalId referenceId = info.GetReferenceId ();
				GidSet::iterator gidIter = currentPrehistoricIds.find (referenceId);
				if (gidIter != currentPrehistoricIds.end ())
				{
					// Inbox script becomes a prehistoric script for the next pass
					newPrehistoricIds.insert (info.GetScriptId ());
					// Delete inbox script depending on the prehistoric id
					DeleteScriptFile (info.GetScriptPath ());
					history.XRemoveDisconnectedScript (info.GetScriptId (), info.GetUnitType ());
				}
			}
			currentPrehistoricIds.swap (newPrehistoricIds);
		} while (!currentPrehistoricIds.empty ());
	}

	// Remove from the missing disconnected script list those ids for which we have added a missing placeholder in the history
	for (MissingList::const_iterator seq = _newMissingPlaceholders.begin (); seq != _newMissingPlaceholders.end (); ++seq)
	{
		std::pair<GlobalId ,Unit::Type> const & missingScript = *seq;
		history.XRemoveDisconnectedScript (missingScript.first, missingScript.second);
		if (missingScript.second == Unit::Set)
			projectDb.XUpdateSender (missingScript.first);
	}

	// Enter file data for the new folders
	for (NewFolderList::const_iterator iter = _newFolders.begin ();
		 iter != _newFolders.end ();
		 ++iter)
	{
		std::pair<GlobalId, UniqueName> const & newFolder = *iter;
		database.XAddForeignFolder (newFolder.first, newFolder.second);
	}

	// Process checkout notifications
	for (auto_vector<CheckOut::List>::const_iterator iter = _checkoutNotifications.begin ();
		 iter != _checkoutNotifications.end ();
		 ++iter)
	{
		CheckOut::List const * notification = *iter;
		Assert (notification != 0);
		GidList const & incomingCheckoutFiles = notification->GetFileList ();
		UserId senderId = notification->GetSenderId ();
		checkedoutFiles.XUpdate (incomingCheckoutFiles, senderId);
	}
}

void Agent::XRefreshUserData (Project::Db const & projectDb)
{
	_thisUserAgent.XRefreshUserData (projectDb);
	_myId = projectDb.XGetMyId ();
}

void Agent::DeleteScriptFile (std::string const & path)
{
	_unpackedScripts.RememberDeleted (path.c_str (), false);	// Path is not a folder path
}
