//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "precompiled.h"
#include "MembershipUpdateVerifier.h"
#include "ScriptHeader.h"
#include "ScriptCommandList.h"
#include "ProjectDb.h"
#include "History.h"
#include "Permissions.h"
#include "MemberInfo.h"
#include "OutputSink.h"

MembershipUpdateVerifier::MembershipUpdateVerifier (ScriptHeader const & hdr, CommandList const & cmdList, bool isQuickVisit)
	: _isQuickVisit (isQuickVisit),
	  _isAddMember (hdr.IsAddMember ()),
	  _isEditMember (hdr.IsEditMember ()),
	  _isFromVersion40 (hdr.IsFromVersion40 ()),
	  _canExecute (true),
	  _confirmConversion (false),
	  _incomingUpdate (0)
{
	CommandList::Sequencer seq (cmdList);
	MemberCmd const & memberCmd = seq.GetMemberCmd ();
	if (_isAddMember)
	{
		NewMemberCmd const & addMemberCmd = seq.GetAddMemberCmd ();
		_incomingUpdate = &addMemberCmd.GetMemberInfo ();
	}
	else if (_isEditMember)
	{
		EditMemberCmd const & editMemberCmd = seq.GetEditMemberCmd ();
		_incomingUpdate = &editMemberCmd.GetNewMemberInfo ();
	}
}

void MembershipUpdateVerifier::CheckUpdate (Project::Db const & projectDb, History::Db const & history, Permissions const & userPermissions)
{
	if (_isAddMember)
	{
		Assert (_incomingUpdate != 0);
		CheckAddMember (projectDb, history);
	}
	else if (_isEditMember)
	{
		Assert (_incomingUpdate != 0);
		CheckEditMember (projectDb, userPermissions);
	}
	// else don't check defect
}

void MembershipUpdateVerifier::CheckAddMember (Project::Db const & projectDb, History::Db const & history)
{
	if (!_isFromVersion40 && history.IsFullSyncExecuted ())
	{
		// Our history is initialized and we are processing add project member command
		// from version 4.5 or higher. Check if we have to send back conversion confirmation script.
		Assert (_incomingUpdate->State ().IsVerified ());
		UserId userId = _incomingUpdate->Id ();
		// Confirm conversion membership update when the user is already present in the
		// membership database, did not convert to the version 4.5 and we don't have any
		// membership change history recorded.
		if (projectDb.IsProjectMember (userId))
		{
			Lineage userLineage;
			history.GetUnitLineage (Unit::Member, userId, userLineage);
			MemberState state = projectDb.GetMemberState (userId);
			_confirmConversion = !state.IsVerified () && (userLineage.Count () == 0);
		}
	}
}

void MembershipUpdateVerifier::CheckEditMember (Project::Db const & projectDb, Permissions const & userPermissions)
{
	UserId changedUserId = _incomingUpdate->Id ();
	if (!projectDb.IsProjectMember (changedUserId))
		return;	// We don't know this member yet -- execute this update

	// Check if we can execute membership update for a known user.
	// Determine if we have take additional actions after this membership
	// update has been executed.
	if (projectDb.IsProjectAdmin ())
	{
		// Administrator verification
		AdminChecks (projectDb);		
	}
	else if (changedUserId == projectDb.GetMyId ())
	{
		// This member verification
		ThisUserChecks (projectDb, userPermissions);
	}
}

void MembershipUpdateVerifier::AdminChecks (Project::Db const & projectDb)
{
	MemberState currentUserState = projectDb.GetMemberState (_incomingUpdate->Id ());
	MemberState updateState = _incomingUpdate->State ();
	if (_incomingUpdate->Description ().IsLicensed ())
	{
		// Licensed user
		if (currentUserState.IsObserver () && updateState.IsVoting ())
		{
			// Licensed user changes his/her state from observer to voting member
			CheckLicensedMember (projectDb);
		}
	}
	else if (currentUserState.IsReceiver ())
	{
		// Un-licensed receiver
		if (currentUserState.IsVoting () && updateState.IsObserver ())
		{
			// Receiver trial end -- send back defect membership update
			_response.reset (new MemberInfo (*_incomingUpdate));
			StateDead  dead (updateState);
			_response->SetState (dead);
		}
	}
}

void MembershipUpdateVerifier::ThisUserChecks (Project::Db const & projectDb, Permissions const & userPermissions)
{
	Assert (_incomingUpdate->Id () == projectDb.GetMyId ());

	UserId thisUserId = projectDb.GetMyId ();
	MemberState currentUserState = projectDb.GetMemberState (thisUserId);
	MemberState updateState = _incomingUpdate->State ();
	if (!currentUserState.IsEqualModuloCoNotify (updateState) && !_isQuickVisit)
	{
		// This user state change -- inform the user
		std::string info ("Administrator has changed your state to ");
		info += updateState.GetDisplayName ();
		info += '.';
		TheOutput.Display (info.c_str ());
	}

	if (currentUserState.IsObserver () && updateState.IsVoting ())
	{
		// Observer to voting member change
		std::unique_ptr<MemberDescription> currentDescription = projectDb.RetrieveMemberDescription (thisUserId);
		if (currentDescription->IsLicensed ())
		{
			// This user is licensed
			CheckLicensedMember (projectDb);
		}
		else if (userPermissions.GetTrialDaysLeft () == 0)
		{
			// This user doesn't have a valid license and trial period is over
			if (currentUserState.IsReceiver () && _incomingUpdate->Description ().IsLicensed ())
			{
				// This user is receiver and update carries a valid license -- allow state change
			}
			else
			{
				// Disallow state change
				RejectUpdate ();
				if (!_isQuickVisit)
				{
					License license (currentDescription->GetLicense ());
					std::string info ("You have received membership update changing your state from observer to voting member.\n\n"
									  "You cannot accept this state change, because ");
					if (license.GetSeatCount () == 0)
					{
						info += "you don't have a valid license and your trial period has ended.\n";
					}
					else
					{
						info += "your license is only valid for version ";
						info += ToString (license.GetVersion ());
						info += " of Code Co-op\n";
					}
					info += "Appropriate information has been sent back to other project members.";
					TheOutput.Display (info.c_str ());
				}
			}
		}
	}
}

void MembershipUpdateVerifier::CheckLicensedMember (Project::Db const & projectDb)
{
	Project::Seats seats (projectDb, _incomingUpdate->License ());
	if (!seats.IsEnoughLicenses ())
	{
		// We cannot allow this change because there is not enough licensed seats
		RejectUpdate ();
		if (!_isQuickVisit)
		{
			std::string info ("You have received membership update changing ");
			info += _incomingUpdate->Description ().GetName ();
			info += "'s state from observer to voting member.\n\n";
			info += "You cannot accept this state change, because there aren't enough licensed seats in the project.";
			TheOutput.Display (info.c_str ());
		}
	}
}

void MembershipUpdateVerifier::RejectUpdate ()
{
	_response.reset (new MemberInfo (*_incomingUpdate));
	Assert (_incomingUpdate->State ().IsVoting ());
	StateObserver observer (_incomingUpdate->State ());
	_response->SetState (observer);
}
