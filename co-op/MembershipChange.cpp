// ----------------------------------
// (c) Reliable Software, 2006 - 2007
// ----------------------------------

#include "precompiled.h"
#include "MembershipChange.h"
#include "History.h"
#include "ProjectDb.h"
#include "MemberInfo.h"
#include "MemberDescription.h"
#include "DispatcherScript.h"
#include "Mailer.h"
#include "GlobalId.h"

XMembershipChange::XMembershipChange (MemberInfo const & updateInfo, Project::Db & projectDb)
	: _newMemberInfo (updateInfo),
	  _oldMemberInfo (projectDb.XRetrieveMemberInfo (_newMemberInfo.Id ())),
	  _projectDb (projectDb),
	  _sendScript (projectDb.XScriptNeeded (true))	// For membership update
{
	Assert (_oldMemberInfo.get () != 0);
	_changesDetected = !_oldMemberInfo->IsIdentical (_newMemberInfo);
}

void XMembershipChange::UpdateProjectMembership ()
{
	Assert (ChangesDetected ());
	if (!_oldMemberInfo->Description ().IsEqual (_newMemberInfo.Description ()))
	{
		_projectDb.XReplaceDescription (_newMemberInfo.Id (), _newMemberInfo.Description ());
	}
	if (!_oldMemberInfo->State ().IsEqual (_newMemberInfo.State ()))
	{
		_projectDb.XChangeState (_newMemberInfo.Id (), _newMemberInfo.State ());
	}
}

void XMembershipChange::BuildScript (History::Db & history)
{
	Assert (ChangesDetected ());
	GlobalId scriptId = _projectDb.XMakeScriptId ();
	std::unique_ptr<ScriptCmd> cmd;
	if (_newMemberInfo.State ().IsDead ())
	{
		_hdr.reset (new ScriptHeader (ScriptKindDeleteMember (),
			_newMemberInfo.Id (),
			_projectDb.XProjectName ()));
		cmd.reset (new DeleteMemberCmd (_newMemberInfo));
	}
	else
	{
		_hdr.reset (new ScriptHeader (ScriptKindEditMember (),
			_newMemberInfo.Id (),
			_projectDb.XProjectName ()));
		cmd.reset (new EditMemberCmd (*_oldMemberInfo, _newMemberInfo));
	}
	_hdr->SetScriptId (scriptId);
	UnitLineage::Type sideLineageType = UnitLineage::Empty;
	if (_sendScript && !_hdr->IsDefectOrRemove ())
	{
		sideLineageType = UnitLineage::Minimal;
		if (!_oldMemberInfo->State ().IsEqual (_newMemberInfo.State ()))
		{
			// Member state change
			MemberState oldState = _oldMemberInfo->State ();
			MemberState newState = _newMemberInfo.State ();
			if (_oldMemberInfo->Id () != _projectDb.XGetMyId () && oldState.IsObserver () && newState.IsVoting ())
			{
				// I'm changing some user state from observer to voting member.
				// Side lineages will include complete trunk lineage of each project
				// member history.
				sideLineageType = UnitLineage::Maximal;
			}
		}
	}
	history.XGetLineages (*_hdr, sideLineageType);
	_hdr->AddComment (BuildComment ());
	_cmdList.push_back (std::move(cmd));
}

void XMembershipChange::StoreInHistory (History::Db & history, AckBox & ackBox) const
{
	if (_newMemberInfo.State ().IsVerified ())
		history.XAddCheckinScript (*_hdr, _cmdList, ackBox);
}

void XMembershipChange::Broadcast (ScriptMailer & mailer,
								   std::unique_ptr<DispatcherCmd> attachment,
								   CheckOut::List const * notification)
{
	if (!_sendScript)
		return;

	Assert (_hdr.get () != 0 && _cmdList.size () != 0);
	GidSet filterOut;
	if (_newMemberInfo.State ().IsReceiver ())
	{
		// Don't tell other receivers about this receiver membership change
		_projectDb.XGetReceivers (filterOut);
		// Don't filter out this receiver 
		filterOut.erase (_newMemberInfo.Id ());
	}
	// Don't send membership update to this user
	filterOut.insert (_projectDb.XGetMyId ());
	if (attachment.get () != 0)
	{
		DispatcherScript dispatcherScript;
		dispatcherScript.AddCmd (std::move(attachment));
		mailer.XMulticast (*_hdr, _cmdList, filterOut, notification, &dispatcherScript);
		if (_newMemberInfo.State ().IsDead () && _newMemberInfo.Id () != _projectDb.XGetMyId ())
		{
			Assert (_projectDb.XGetAdminId () == _projectDb.XGetMyId ());
			// Unicast defect script to the user removed by admin
			std::unique_ptr<MemberDescription> defectedMember = _projectDb.XRetrieveMemberDescription (_newMemberInfo.Id ());
			mailer.XUnicast (*_hdr, _cmdList, *defectedMember, notification, &dispatcherScript);
		}
	}
	else
	{
		mailer.XMulticast (*_hdr, _cmdList, filterOut, notification);
	}
}

void XMembershipChange::BuildFutureDefect (
		ScriptMailer & mailer, 
		History::Db & history,
		std::string & defectFilename,
		std::vector<unsigned char> & defectScript)
{
	Assert (_sendScript);
	Assert (ChangesDetected ());

	_hdr.reset (new ScriptHeader (ScriptKindDeleteMember (),
								_newMemberInfo.Id (),
								_projectDb.XProjectName ()));
	GlobalIdPack scriptId (_newMemberInfo.Id (), 1); // script with ordinal 1 sent by the new user
	_hdr->SetScriptId (scriptId);
	history.XGetLineages (*_hdr, UnitLineage::Empty);

	MemberNameTag tag (_newMemberInfo.Name (), _newMemberInfo.Id ());
	MembershipUpdateComment comment (tag, "defects from the project");
	_hdr->AddComment (comment);

	std::unique_ptr<ScriptCmd> cmd (new DeleteMemberCmd (_newMemberInfo));
	_cmdList.push_back (std::move(cmd));

	GidSet filterOut;
	filterOut.insert (_newMemberInfo.Id ());

	DispatcherScript dispatcherScript;
	std::unique_ptr<DispatcherCmd> removeInviteeAddr (
			new AddressChangeCmd (_newMemberInfo.HubId (),
								  _newMemberInfo.GetUserId (),
								  std::string (),
								  std::string ()));
	dispatcherScript.AddCmd (std::move(removeInviteeAddr));

	mailer.XFutureMulticast (
				*_hdr, 
				_cmdList, 
				filterOut, 
				dispatcherScript,
				_newMemberInfo.Id (),
				defectFilename,
				defectScript);
}

std::string XMembershipChange::BuildComment () const
{
	std::string action;
	bool const isThisUserChanged = (_newMemberInfo.Id () == _projectDb.GetMyId ());

	if (!_newMemberInfo.State ().IsEqual (_oldMemberInfo->State ()))
	{
		MemberState newState = _newMemberInfo.State ();
		MemberState oldState = _oldMemberInfo->State ();
		if (newState.IsDead ())
		{
			if (isThisUserChanged)
				action = "defects from the project";
			else
				action = "is removed from the project by the administrator";
		}
		else if (newState.IsCheckoutNotification () != oldState.IsCheckoutNotification ())
		{
			if (isThisUserChanged)
			{
				action = newState.IsCheckoutNotification () ? "starts" : "stops";
				action += " checkout notifications";
			}
			else
			{
				action = "checkout notifications are ";
				action += newState.IsCheckoutNotification () ? "started" : "stopped";
				action += " by the administrator";
			}
		}
		else 
		{
			if (isThisUserChanged)
				action = "changes his/her state from ";
			else
				action = "state is changed by the project administrator from ";
			action += oldState.GetDisplayName ();
			action += " to ";
			action += newState.GetDisplayName ();
		}
	}
	else if (_oldMemberInfo->License () != _newMemberInfo.License ())
	{
		action = "changes his/her license";
	}
	else if (!IsNocaseEqual (_newMemberInfo.HubId (), _oldMemberInfo->HubId ()))
	{
		if (isThisUserChanged)
		{
			// This user Hub's Email Address change
			action = "changes his/her hub's email address";
		}
		else
		{
			Assert (_projectDb.IsProjectAdmin ());
			action = "hub email address is changed by the project administrator";
		}
	}
	else // user description
	{
		if (isThisUserChanged)
			action = "changes his/her member description";
		else
			action = "member description is changed by the project administrator";
	}

	MemberNameTag tag (_oldMemberInfo->Name (), _newMemberInfo.Id ());
	MembershipUpdateComment comment (tag, action);

	return comment;
}