//----------------------------------
// (c) Reliable Software 2002 - 2007
// ---------------------------------

#include "precompiled.h"
#include "ScriptPropertySheet.h"
#include "ScriptProps.h"
#include "GlobalId.h"
#include "MemberInfo.h"
#include "License.h"

#include <Sys/Synchro.h>
#include <Graph/Cursor.h>
#include <TimeStamp.h>
#include <StringOp.h>

//
// Property page controllers
//

bool MissingScriptPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_sender.Init (GetWindow (), IDC_SCRIPT_SENDER);
	_senderId.Init (GetWindow (), IDC_USER_ID);
	_senderHubId.Init (GetWindow (), IDC_HUB_ID);
	_scriptId.Init (GetWindow (), IDC_SCRIPTID);
	_scriptType.Init (GetWindow (), IDC_SCRIPT_TYPE);
	_chunkCount.Init (GetWindow (), IDC_CHUNK_COUNT);
	_maxChunkSize.Init (GetWindow (), IDC_MAX_CHUNK);
	_receivedChunkCount.Init (GetWindow (), IDC_RECEIVED_CHUNK_COUNT);
	_resendRecipients.Init (GetWindow (), IDC_REQUEST_RECIPIENTS);
	_nextRecipientCaption.Init (GetWindow (), IDC_NEXT_RESEND_CAPTION);
	_nextRecipient.Init (GetWindow (), IDC_NEXT_RECIPIENT);


	std::unique_ptr<MemberInfo> senderInfo = _props.RetrieveSenderInfo ();
	_sender.SetString (senderInfo->Name ().c_str ());
	_senderId.SetString (ToHexString (senderInfo->Id ()).c_str ());
	_senderHubId.SetString (senderInfo->HubId ().c_str ());
	GlobalIdPack pack (_props.GetScriptId ());
	_scriptId.SetString (pack.ToString ().c_str ());
	Unit::Type scriptType = _props.GetScriptType ();
	if (scriptType == Unit::Member)
	{
		_scriptType.SetString ("membership update");
	}
	else
	{
		Assert (scriptType == Unit::Set);
		if (_props.IsFullSynchResendRequest ())
			_scriptType.SetString ("full synch");
		else
			_scriptType.SetString ("file edit changes");
	}
	_chunkCount.SetString (ToString (_props.GetPartCount ()).c_str ());
	if (_props.GetPartCount () == 1)
		_maxChunkSize.SetString ("n/a");
	else
		_maxChunkSize.SetString (FormatFileSize (_props.GetMaxChunkSize ()).c_str ());

	_receivedChunkCount.SetString (ToString (_props.GetReceivedPartCount ()).c_str ());
	_resendRecipients.AddProportionalColumn (38, "Name");
	_resendRecipients.AddProportionalColumn (48, "Hub's Email Address");
	_resendRecipients.AddProportionalColumn (10, "User Id");
	// When resend request list is not exchausted then the last
	// member on the list is the next resend request recipient
	ScriptProps::MemberSequencer seq (_props);
	std::string lastUserId;
	if (!_props.IsResendListExchausted ())
	{
		lastUserId = ToHexString (_props.NextRecipientId ());
	}
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		if (!lastUserId.empty () && lastUserId == seq.GetStrId ())
			break;

		int row = _resendRecipients.AppendItem (seq.GetName ());
		_resendRecipients.AddSubItem (seq.GetHubId (), row, 1);
		_resendRecipients.AddSubItem (seq.GetStrId (), row, 2);
	}

	if (_props.IsResendListExchausted ())
	{
		_nextRecipientCaption.SetText ("Resend requests exhausted.");
		_nextRecipient.Hide ();
	}
	else
	{
		std::string caption ("Next resend request will be send to the following project member on ");
		PackedTimeStr nextTime (_props.GetNextResendTime ());
		caption += nextTime.ToString ();
		_nextRecipientCaption.SetText (caption.c_str ());
		_nextRecipient.AddProportionalColumn (38, "Name");
		_nextRecipient.AddProportionalColumn (48, "Hub's Email Address");
		_nextRecipient.AddProportionalColumn (10, "User Id");
		Assert (!seq.AtEnd ());
		int row = _nextRecipient.AppendItem (seq.GetName ());
		_nextRecipient.AddSubItem (seq.GetHubId (), row, 1);
		_nextRecipient.AddSubItem (seq.GetStrId (), row, 2);
	}
	// Disable Cancel button and change OK to Close
	CancelToClose ();
	return true;
}

bool ScriptGeneralPageHndlr::OnInitDialog () throw (Win::Exception)
{
	// All general script pages have to have the following read-only edit fields:
	_scriptId.Init (GetWindow (), IDC_SCRIPTID);
	_date.Init (GetWindow (), IDC_SCRIPT_DATE);
	_sender.Init (GetWindow (), IDC_SCRIPT_SENDER);
	_senderId.Init (GetWindow (), IDC_USER_ID);
	_senderHubId.Init (GetWindow (), IDC_HUB_ID);

	GlobalIdPack pack (_props.GetScriptId ());
	_scriptId.SetString (pack.ToString ().c_str ());
	StrTime timeStamp (_props.GetScriptTimeStamp ());
	_date.SetString (timeStamp.GetString ());
	std::unique_ptr<MemberInfo> senderInfo = _props.RetrieveSenderInfo ();
	_sender.SetString (senderInfo->Name ().c_str ());
	_senderId.SetString (ToHexString (senderInfo->Id ()).c_str ());
	_senderHubId.SetString (senderInfo->HubId ().c_str ());
	// Disable Cancel button and change OK to Close
	CancelToClose ();
	return true;
}

bool ChangeScriptHeaderPageHndlr::OnInitDialog () throw (Win::Exception)
{
	ScriptGeneralPageHndlr::OnInitDialog ();

	_comment.Init (GetWindow (), IDC_CHECKIN_COMMENT_EDIT);
	_lineage.Init (GetWindow (), IDC_LINEAGE);
	_memberFrame.Init (GetWindow (), IDC_MEMBER_FRAME);
	_members.Init (GetWindow (), IDC_MEMBER_LIST);

	_comment.SetString (_props.GetCheckinComment ().c_str ());
	
	_lineage.AddProportionalColumn (16, "Script Id");
	_lineage.AddProportionalColumn (80, "Created By");
	for (ScriptProps::LineageSequencer seq (_props); !seq.AtEnd (); seq.Advance ())
	{
		int row = _lineage.AppendItem (seq.GetScriptId ());
		_lineage.AddSubItem (seq.GetSender (), row, 1);
	}

	if (_props.IsFromHistory ())
	{
		// Ack list
		if (_props.IsAwaitingFinalAck ())
			_memberFrame.SetText ("Awaiting final acknowledgement from the script author: ");
		else if (_props.IsOverdue ())
			_memberFrame.SetText ("Overdue! The following members didn't acknowledge this script for more then two weeks: ");
		else
			_memberFrame.SetText ("Awaiting acknowledgement from the following members: ");
		ScriptProps::MemberSequencer seq (_props);
		if (seq.AtEnd ())
		{
			_members.AddProportionalColumn (96, "");
			_members.AppendItem ("All voting members have acknowledged receipt of this script");
		}
		else
		{
			_members.AddProportionalColumn (38, "Name");
			_members.AddProportionalColumn (48, "Hub's Email Address");
			_members.AddProportionalColumn (10, "User Id");
			for ( ; !seq.AtEnd (); seq.Advance ())
			{
				int row = _members.AppendItem (seq.GetName ());
				_members.AddSubItem (seq.GetHubId (), row, 1);
				_members.AddSubItem (seq.GetStrId (), row, 2);
			}
		}
	}
	else
	{
		// Recipient list
		_memberFrame.SetText ("Script recipients: ");
		ScriptProps::MemberSequencer seq (_props);
		_members.AddProportionalColumn (38, "Name");
		_members.AddProportionalColumn (48, "Hub's Email Address");
		_members.AddProportionalColumn (10, "User Id");
		for ( ; !seq.AtEnd (); seq.Advance ())
		{
			int row = _members.AppendItem (seq.GetName ());
			_members.AddSubItem (seq.GetHubId (), row, 1);
			_members.AddSubItem (seq.GetStrId (), row, 2);
		}
	}
	return true;
}

Notify::Handler * FileDetailsPageHndlr::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_fileDisplay.IsHandlerFor (idFrom))
		return &_fileDisplay;
	else
		return 0;
}

bool FileDetailsPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_fileDisplay.Init (GetWindow (), IDC_FILE_DETAILS_LIST);
	_fileDisplay.Refresh ();
	_open.Init (GetWindow (), IDC_FILE_DETAILS_OPEN);
	_openAll.Init (GetWindow (), IDC_FILE_DETAILS_OPENALL);
	return true;
}

bool FileDetailsPageHndlr::OnDlgControl (unsigned id, unsigned notifyCode) throw ()
{
	if (Win::SimpleControl::IsClicked (notifyCode))
	{
		Cursor::Hourglass hourglass;
		Cursor::Holder working (hourglass);

		if (id == IDC_FILE_DETAILS_OPEN)
		{
			_fileDisplay.OpenSelected ();
		}
		else if (id == IDC_FILE_DETAILS_OPENALL)
		{
			_fileDisplay.OpenAll ();
		}
		return true;
	}
	return false;
}

bool MembershipUpdatePageHndlr::OnInitDialog () throw (Win::Exception)
{
	ScriptGeneralPageHndlr::OnInitDialog ();

	_comment.Init (GetWindow (), IDC_CHECKIN_COMMENT_EDIT);
	_lineage.Init (GetWindow (), IDC_LINEAGE);

	_comment.SetString (_props.GetCheckinComment ().c_str ());

	_lineage.AddProportionalColumn (16, "Script Id");
	_lineage.AddProportionalColumn (80, "Created By");
	for (ScriptProps::LineageSequencer seq (_props); !seq.AtEnd (); seq.Advance ())
	{
		int row = _lineage.AppendItem (seq.GetScriptId ());
		_lineage.AddSubItem (seq.GetSender (), row, 1);
	}

	_changes.Init (GetWindow (), IDC_MEMBER_DATA_CHANGES);
	_changes.AddProportionalColumn (20, "");

	if (_props.IsAddMember () || _props.IsDefectOrRemove ())
	{
		_changes.AddProportionalColumn (74, "Script Change");

		MemberInfo const & updatedInfo = _props.GetUpdatedMemberInfo ();
		int row = _changes.AppendItem ("Name");
		_changes.AddSubItem (updatedInfo.Name ().c_str (), row, 1);

		row = _changes.AppendItem ("Hub's Email Address");
		_changes.AddSubItem (updatedInfo.HubId ().c_str (), row, 1);

		row = _changes.AppendItem ("User Id");
		_changes.AddSubItem (ToHexString (updatedInfo.Id ()).c_str (), row, 1);

		row = _changes.AppendItem ("State");
		_changes.AddSubItem (updatedInfo.GetStateDisplayName (), row, 1);

		row = _changes.AppendItem ("Comment");
		_changes.AddSubItem (updatedInfo.Comment ().c_str (), row, 1);

		row = _changes.AppendItem ("License");
		License update (updatedInfo.License ());
		if (update.IsValid ())
		{
			std::string info (update.GetLicensee ());
			info += " (";
			info += ToString (update.GetSeatCount ());
			info += " seats)";
			_changes.AddSubItem (info.c_str (), row, 1);
		}
		else
		{
			_changes.AddSubItem ("Invalid", row, 1);
		}
	}
	else
	{
		_changes.AddProportionalColumn (39, "Current");
		_changes.AddProportionalColumn (39, "Script Change");

		MemberInfo const & currentInfo = _props.GetCurrentMemberInfo ();
		MemberInfo const & updatedInfo = _props.GetUpdatedMemberInfo ();
		int row = _changes.AppendItem ("Name");
		_changes.AddSubItem (currentInfo.Name ().c_str (), row, 1);
		_changes.AddSubItem (updatedInfo.Name ().c_str (), row, 2);

		row = _changes.AppendItem ("Hub's Email Address");
		_changes.AddSubItem (currentInfo.HubId ().c_str (), row, 1);
		_changes.AddSubItem (updatedInfo.HubId ().c_str (), row, 2);

		row = _changes.AppendItem ("User Id");
		_changes.AddSubItem (ToHexString (currentInfo.Id ()).c_str (), row, 1);
		_changes.AddSubItem (ToHexString (updatedInfo.Id ()).c_str (), row, 2);

		row = _changes.AppendItem ("State");
		_changes.AddSubItem (currentInfo.GetStateDisplayName (), row, 1);
		_changes.AddSubItem (updatedInfo.GetStateDisplayName (), row, 2);

		row = _changes.AppendItem ("Comment");
		_changes.AddSubItem (currentInfo.Comment ().c_str (), row, 1);
		_changes.AddSubItem (updatedInfo.Comment ().c_str (), row, 2);

		row = _changes.AppendItem ("License");
		License current (currentInfo.License ());
		if (current.IsValid ())
		{
			std::string info (current.GetLicensee ());
			info += " (";
			info += ToString (current.GetSeatCount ());
			info += " seats)";
			_changes.AddSubItem (info.c_str (), row, 1);
		}
		else
		{
			_changes.AddSubItem ("Invalid", row, 1);
		}

		License update (updatedInfo.License ());
		if (update.IsValid ())
		{
			std::string info (update.GetLicensee ());
			info += " (";
			info += ToString (update.GetSeatCount ());
			info += " seats)";
			_changes.AddSubItem (info.c_str (), row, 2);
		}
		else
		{
			_changes.AddSubItem ("Invalid", row, 2);
		}
	}
	return true;
}

bool CtrlScriptPageHndlr::OnInitDialog () throw (Win::Exception)
{
	ScriptGeneralPageHndlr::OnInitDialog ();

	_caption.Init (GetWindow (), IDC_CAPTION);
	_comment.Init (GetWindow (), IDC_CHECKIN_COMMENT_EDIT);

	_comment.SetString (_props.GetCheckinComment ().c_str ());

	std::string caption;
	BuildCaption (caption);
	_caption.SetText (caption.c_str ());
	return true;
}

void CtrlScriptPageHndlr::BuildCaption (std::string & caption) const
{
	if (_props.IsAck ())
	{
		std::vector<std::pair<GlobalId, bool> > const & acks = _props.GetAcks ();
		caption = "The sender acknowledges reception of the following script(s):\n\n";
		std::string infoMakeRef ("The sender announces new reference version(s):\n\n");
		bool makeRefSeen = false;

		for (std::vector<std::pair<GlobalId, bool> >::const_iterator iter = acks.begin (); 
			iter != acks.end (); 
			++iter)
		{
			std::pair<GlobalId, bool> const & ack = *iter;
			GlobalIdPack pack (ack.first);
			if (ack.second)
			{		
				infoMakeRef += pack.ToBracketedString ();
				infoMakeRef += ' ';
				makeRefSeen = true;
			}
			else
			{
				caption += pack.ToBracketedString ();
				caption += ' ';
			}

		}
		if (makeRefSeen)
		{
			caption += "\n\n";
			caption += infoMakeRef;
		}
	}
	else if (_props.IsJoinRequest ())
	{
		MemberInfo const & info = _props.GetUpdatedMemberInfo ();
		caption = "The user ";
		caption += info.Description ().GetName ();
		caption += " wants to join the project ";
		caption += _props.GetProjectName ();
		caption += " as the ";
		caption += info.GetStateDisplayName ();
		caption += ".";
		caption += "\nThe user is connected to the hub ";
		caption += info.Description ().GetHubId ();
		caption += ".";
		License licPack (info.License ());
		if (licPack.IsValid ())
		{
			caption += "\nThe user license: ";
			caption += licPack.GetLicensee ();
			caption += " (";
			caption += ToString (licPack.GetSeatCount ());
			caption += " seats).";
		}
		else
		{
			caption += "\nThe user doesn't have a valid license.";
		}
	}
	else if (_props.IsScriptResendRequest ())
	{
		if (_props.IsProjectVerificationRequest ())
		{
			caption = "Sender requests project verification.\n";
			caption += "Sender's last confirmed script id ";
			GlobalIdPack pack (_props.GetRequestedScriptId ());
			caption += pack.ToBracketedString ();
			caption += "\nSender knows about the following defected project members:\n    ";
			GidList const & knownDeadMembers = _props.GetKnownDeadMembers ();
			unsigned count = knownDeadMembers.size ();
			for (unsigned i = 0; i < count; ++i)
			{
				caption += ToHexString (knownDeadMembers [i]);
				if (i < count - 1)
					caption += ", ";
			}
		}
		else
		{
			caption = "The sender requests the ";
			GlobalIdPack pack (_props.GetRequestedScriptId ());
			caption += pack.ToBracketedString ();
			caption += " script.";
			FormatChunkInfo (caption);
		}
	}
	else if (_props.IsFullSynchResendRequest ())
	{
		caption = "The sender requests the full synch script.";
		FormatChunkInfo (caption);
	}
	else if (_props.IsFullSynch ())
	{
		caption = "Project full synch script initializing new project enlistment.";
	}
	else if (_props.IsPackage ())
	{
		caption = "Package containig multiple scripts.";
	}
	else
	{
		caption = "Unknow control script kind.";
	}
}

void CtrlScriptPageHndlr::FormatChunkInfo (std::string & caption) const
{
	unsigned partCount = _props.GetPartCount ();
	unsigned partNumber = _props.GetPartNumber ();
	if (partCount == partNumber)
		return;

	caption += "\nThe script was split into ";
	caption += ToString (partCount);
	caption += " chunks and the sender is missing chunk number ";
	caption += ToString (partNumber);
	caption += ".\nMaximum chunk size is set to ";
	caption += ToString (_props.GetMaxChunkSize ());
	caption += " bytes.";
}

//
// Script property sheet controller set
//

ScriptPropertyHndlrSet::ScriptPropertyHndlrSet (ScriptProps const & props)
	: PropPage::HandlerSet (props.GetCaption ()),
	  _missingScriptPageHndlr (props),
	  _scriptHdrPageHndlr (props),
	  _membershipUpdatePageHdnlr (props),
	  _ctrlScriptPageHdnlr (props)
{
	// Add controllers appropriate for a given script
	if (props.IsMissing ())
	{
		AddHandler (_missingScriptPageHndlr, "General");
	}
	else if (props.IsSetChange ())
	{
		AddHandler (_scriptHdrPageHndlr, "General");
		Assert (!props.IsMissing ());
		FileDisplayTable const & fileTable = props.GetScriptFileTable ();
		if (fileTable.GetFileSet ().size () != 0)
		{
			_filePageHndlr.reset (new FileDetailsPageHndlr (fileTable));
			AddHandler (*_filePageHndlr, "Files");
		}
	}
	else if (props.IsMembershipUpdate ())
	{
		AddHandler (_membershipUpdatePageHdnlr, "General");
	}
	else
	{
		Assert (props.IsCtrl ());
		AddHandler (_ctrlScriptPageHdnlr, "General");
	}
}
