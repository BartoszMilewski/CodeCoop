//------------------------------------
//  (c) Reliable Software, 2004 - 2007
//------------------------------------

#include "precompiled.h"
#include "AckBox.h"
#include "Mailer.h"
#include "ProjectDb.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"

void AckBox::RememberMakeRef (Unit::ScriptId const & acknowledgedScriptId)
{
	AddAck (_mustAck, gidInvalid, acknowledgedScriptId, true);
}

void AckBox::RememberMakeRef (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId)
{
	AddAck (_mustAck, recipientId, acknowledgedScriptId, true);
}

void AckBox::RememberAck (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId, bool mustAcknowledge)
{
	if (mustAcknowledge)
		AddAck (_mustAck, recipientId, acknowledgedScriptId);
	else if (!IsPresent (_mustAck, recipientId, acknowledgedScriptId))
		AddAck (_canAck, recipientId, acknowledgedScriptId);
}

void AckBox::RemoveAckForScript (UserId recipientId, Unit::ScriptId const & acknowledgedScriptId)
{
	AckKey recipient (recipientId, acknowledgedScriptId.Type ());
	AckScriptId scriptId (acknowledgedScriptId.Gid (), false);
	AckRecipients::iterator recipientIter = _mustAck.find (recipient);
	if (recipientIter != _mustAck.end ())
	{
		AckList & ackList = recipientIter->second;
		AckList::iterator ackIter = std::find_if (ackList.begin (), ackList.end (), IsEqualScriptId (scriptId));
		if (ackIter != ackList.end ())
		{
			ackList.erase (ackIter);
			if (ackList.empty ())
				_mustAck.erase (recipientIter);
		}
	}
}

void AckBox::XSendAck (ScriptMailer & mailer,
					   Project::Db & projectDb,
					   ScriptHeader & ackHdr,
					   CheckOut::List const * checkoutNotification) const
{
	for (AckRecipients::const_iterator iter = _mustAck.begin (); iter != _mustAck.end (); ++iter)
	{
		std::string comment ("Acknowledgments: ");
		AckRecipient const & ackRecipient = *iter;
		GlobalId ackRecipientId = ackRecipient.first._recipientId;
		if (ackRecipientId != gidInvalid)
		{
			if (ackRecipientId == projectDb.XGetMyId ())
				continue;	// Don't acknowledge your own scripts
			if (!projectDb.XIsProjectMember (ackRecipientId))
				continue;	// Script for this project but from the sender we don't know yet -- don't send ack script
			if (projectDb.XGetMemberState (ackRecipientId).IsDead ())
				continue;	// Don't acknowledge scripts from defected project members -- can happen for re-send scripts
		}

		if (!projectDb.XScriptNeeded (ackRecipient.first._unitType == Unit::Member))
			continue;	// Don't acknowledge because you are the only one project member left or
						// you are project observer and this is not  a membership update acknowledgment

		ackHdr.AddScriptId (projectDb.XMakeScriptId ());
		ackHdr.SetUnitType (ackRecipient.first._unitType);
		Assert (ackHdr.GetUnitType () == Unit::Set || ackHdr.GetUnitType () == Unit::Member);

		CommandList cmdList;
		AddAckCmds (cmdList, ackRecipient.second);
		UpdateScriptComment (comment, ackRecipient.second);

		AckRecipients::const_iterator canIter = _canAck.find (ackRecipient.first);
		if (canIter != _canAck.end ())
		{
			AddAckCmds (cmdList, (*canIter).second);
			comment += ", ";
			UpdateScriptComment (comment, (*canIter).second);
		}

		ackHdr.AddComment (comment);
		if (ackRecipient.first._recipientId == gidInvalid)
		{
			mailer.XBroadcast (ackHdr, cmdList, checkoutNotification);
		}
		else
		{
			std::unique_ptr<MemberDescription> recipient =
				projectDb.XRetrieveMemberDescription (ackRecipient.first._recipientId);
			mailer.XUnicast (ackHdr, cmdList, *recipient, checkoutNotification);
		}
	}
}

void AckBox::AddAckCmds (CommandList & cmdList, AckList const & ackList) const
{
	for (AckList::const_iterator iter = ackList.begin (); iter != ackList.end (); ++iter)
	{
		AckScriptId const & ack = *iter;
		std::unique_ptr<ScriptCmd> cmd;
		if (ack._makeRef)
			cmd.reset (new MakeReferenceCmd (ack._scriptId));
		else
			cmd.reset (new AckCmd (ack._scriptId));
		cmdList.push_back (std::move(cmd));
	}
}

void AckBox::UpdateScriptComment (std::string & comment, AckList const & ackList) const
{
	Assert (ackList.size () != 0);
	Assert (comment.length () != 0);
	AckList::const_iterator iter = ackList.begin ();
	while (iter != ackList.end ())
	{
		AckScriptId const & ack = *iter;
		comment += GlobalIdPack (ack._scriptId).ToString ();
		++iter;
		if (iter != ackList.end ())
			comment += ", ";
	}
}

bool AckBox::IsPresent (AckRecipients const & list,
						UserId recipientId,
						Unit::ScriptId const & acknowledgedScriptId,
						bool makeRef) const
{
	AckKey recipient (recipientId, acknowledgedScriptId.Type ());
	AckScriptId scriptId (acknowledgedScriptId.Gid (), makeRef);
	AckRecipients::const_iterator iter = list.find (recipient);
	if (iter != list.end ())
	{
		AckList const & ackList = (*iter).second;
		AckList::const_iterator ackIter =
			std::find_if (ackList.begin (), ackList.end (), IsEqualScriptId (scriptId));
		return ackIter != ackList.end ();
	}
	return false;
}

void AckBox::AddAck (AckRecipients & list,
					 UserId recipientId,
					 Unit::ScriptId const & acknowledgedScriptId,
					 bool makeRef)
{
	// Get already recorded acknowledged scripts for this recipient
	AckKey recipient (recipientId, acknowledgedScriptId.Type ());
	AckScriptId scriptId (acknowledgedScriptId.Gid (), makeRef);
	AckRecipients::iterator iter = list.find (recipient);
	if (iter != list.end ())
	{
		// Check for duplicates
		AckList & ackList = (*iter).second;
		AckList::const_iterator ackIter =
			std::find_if (ackList.begin (), ackList.end (), IsEqualScriptId (scriptId));
		if (ackIter == ackList.end ())
			ackList.push_back (scriptId);
	}
	else
	{
		// Add new acknowledgement recipient
		AckList ackList;
		ackList.push_back (scriptId);
		list.insert (std::make_pair(recipient, ackList));
	}
}
