//-----------------------------------
// (c) Reliable Software 2006
//-----------------------------------

#include "precompiled.h"
#include "JoinContext.h"
#include "Mailbox.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "ReceiverJoinDlg.h"
#include "Catalog.h"
#include "Crypt.h"
#include "License.h"

JoinContext::~JoinContext ()
{
	if (!_invitationFileName.empty ())
	{
		TmpPath tmpPath;
		File::DeleteNoEx (tmpPath.GetFilePath (_invitationFileName));
	}
}

bool JoinContext::Init (Mailbox::Db & mailBox)
{
	// Retrieve join script
	std::unique_ptr<ScriptHeader> joinHdr;
	std::unique_ptr<CommandList> joinCmdList;
	mailBox.RetrieveJoinRequest (joinHdr, joinCmdList);
	Assert (joinHdr->IsJoinRequest ());
	Assert (joinCmdList->size () == 1);
	_scriptId = joinHdr->ScriptId ();
	CommandList::Sequencer seq (*joinCmdList);
	JoinRequestCmd const & joinRequestCmd = seq.GetJoinRequestCmd ();
	// Make a local editable copy
	_joineeInfo = joinRequestCmd.GetMemberInfo ();
	Assert (_joineeInfo.GetMostRecentScript () == gidInvalid && _joineeInfo.GetPreHistoricScript () == gidInvalid);

	if (joinRequestCmd.IsFromVersion40 ())
	{
		std::string info ("Cannot process join request from the older Code Co-op version.\n\n");
		info += "Ask ";
		info += _joineeInfo.Description ().GetName ();
		info += " to defect from the enlistment awaiting the full sync script,\n";
		info += "upgrade to the latest Code Co-op version and join the project again.";
		TheOutput.Display (info.c_str ());
		return false;
	}
	else
		return true;
}

bool JoinContext::WillAccept (std::string const & projName)
{
	MemberDescription const & joineeDescription = GetJoineeDescription ();
	MemberState joinState = GetJoineeInfo ().State ();
	bool accept = false;
	std::string info ("Do you want to accept the following join request:\n\n");
	info += joineeDescription.GetName ();
	info += " (";
	info += joineeDescription.GetHubId ();
	if (_adminState.IsDistributor ())
	{
		info += ")\nas Receiver or Voting Member";
		info += " in the project ";
		info += projName;
		// Display dialog asking voting/receiver, trial/licensed
		// assuming receiver, trial
		ReceiverJoinData dlgData (info, joineeDescription.GetName (), joineeDescription.GetHubId ());
		ReceiverJoinCtrl ctrl (_distributorPool, dlgData);
		if (ThePrompter.GetData (ctrl))
		{
			// Always copy administrator's no branching flag
			if (dlgData.IsReceiver ())
			{
				joinState.MakeReceiver (_adminState.NoBranching ());
				if (dlgData.IsTrial ())
				{
					_joineeInfo.SetLicense (std::string ());
				}
				else
				{
					_joineeInfo.SetLicense (_distributorPool.NewLicense (_log));
				}
			}
			else
			{
				// Mark full members as distributors
				Assert (dlgData.IsFullMember ());
				joinState.SetDistributor (true);
				joinState.SetNoBranching (_adminState.NoBranching ());
			}
			SetJoineeState (joinState);
			accept = true;
		}
	}
	else
	{
		info += ") as ";
		info += joinState.GetDisplayName ();
		info += " in the project ";
		info += projName;
		// Do you want to accept?
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
												   Out::PromptStyle (Out::YesNoCancel,
																	 Out::Yes,
																	 Out::Question));
		if (userChoice == Out::Cancel)
			return false;

		accept = (userChoice == Out::Yes);
	}

	if (!accept)
	{
		MemberDescription const & joineeDescription = GetJoineeDescription ();
		MemberState joinState = GetJoineeInfo ().State ();
		std::string info ("You have rejected a join request from:\n\n");
		info += joineeDescription.GetName ();
		info += " (";
		info += joineeDescription.GetHubId ();
		info += ") as ";
		info += joinState.GetDisplayName ();
		info += " in the project ";
		info += projName;

		info += "\n\nAre you sure?\n\n";
		info += "If so, the join request will be deleted from your inbox\n"
			"You should still inform this person not to wait for a full synch\n"
			"and to defect from the (empty) project.";
		// Do you want to delete?
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
			Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
		DeleteScript (userChoice == Out::Yes);
	}

	return accept;
}
