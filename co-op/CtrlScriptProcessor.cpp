//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CtrlScriptProcessor.h"
#include "ProjectDb.h"
#include "Mailer.h"
#include "Sidetrack.h"
#include "CmdExec.h"
#include "ScriptCommandList.h"
#include "ScriptHeader.h"

bool CtrlScriptProcessor::XRememberResendRequest (ScriptHeader const & hdr, CommandList const & cmdList)
{
	return _sidetrack.XRememberResendRequest (hdr, cmdList);
}

void CtrlScriptProcessor::XExecuteScript (ScriptHeader const & hdr,
										  CommandList const & cmdList,
										  History::Db & history,
										  AckBox & ackBox,
										  CheckOut::List const * notification)
{
	Assert (hdr.IsControl ());
	GlobalIdPack scriptId (hdr.ScriptId ());
	UserId senderId = scriptId.GetUserId ();
	bool isSenderReceiver = XIsMemberReceiver (senderId);
	Unit::Type unitType = hdr.GetUnitType ();

	dbg << "Executing Control Script" << std::endl;
	for (CommandList::Sequencer seq (cmdList); !seq.AtEnd (); seq.Advance ())
	{
		CtrlCmd const & ctrlCmd = seq.GetCtrlCmd ();
		std::unique_ptr<CmdCtrlExec> exec = ctrlCmd.CreateExec (history, senderId);
		exec->Do (unitType, ackBox, isSenderReceiver);
		// After executing the script we may have to send something back to the sender
		if (!_projectDb.XIsProjectMember (senderId))
			continue;

		ScriptHeader & outHdr = exec->GetScriptHeader ();
		std::unique_ptr<MemberDescription> sender = _projectDb.XRetrieveMemberDescription (senderId);
		if (ctrlCmd.GetType () == typeResendRequest)
		{
			// There is a script to be sent out as the result of exec actions
			if (outHdr.ScriptId () == gidInvalid)
				continue;	// We don't have the requested script

			CommandList const & outCmdList = exec->GetCommandList ();
			if (isSenderReceiver)
			{
				// Receiver requesting a missing script - be alert!
				for (CommandList::Sequencer seq (outCmdList); !seq.AtEnd (); seq.Advance ())
				{
					ScriptCmdType cmdType = seq.GetCmd ().GetType ();
					if (cmdType == typeNewMember ||
						cmdType == typeEditMember ||
						cmdType == typeDeleteMember)
					{
						// Receiver is requesting a membership update script.
						// This is OK when the membership update is for the full member.
						MemberCmd const & memberCmd = seq.GetMemberCmd ();
						MemberState changedUserState = _projectDb.XGetMemberState (memberCmd.GetUserId ());
						if (changedUserState.IsReceiver () && memberCmd.GetUserId () != senderId)
						{
							Assert (!"Receiver is requesting a membership update of another receiver.");
							continue;	// Don't resend the requested script and continue with the next control command
						}
					}
				}
			}
			// Re-send script has no side lineages
			outHdr.CopyChunkInfo (hdr); // XUnicast will send only the requested chunk
			_mailer.XUnicast (outHdr, outCmdList, *sender, notification);
		}
		else if (ctrlCmd.GetType () == typeVerificationRequest)
		{
			Assert (outHdr.IsVerificationPackage ());
			outHdr.SetScriptId (_projectDb.XMakeScriptId ());
			outHdr.SetProjectName (_projectDb.XProjectName ());
			_mailer.XUnicast (outHdr, exec->GetScriptList (), *sender, notification);
		}
	}
}

bool CtrlScriptProcessor::XIsMemberReceiver (UserId uid) const
{
	MemberState senderState = _projectDb.XGetMemberState (uid);
	return senderState.IsReceiver ();
}

