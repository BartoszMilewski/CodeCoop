//----------------------------------
// (c) Reliable Software 2000 - 2006
//----------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "Recipient.h"
#include "LocalRecipient.h"
#include "MailTruck.h"
#include "ScriptInfo.h"
#include "Catalog.h"
#include "ProjectData.h"

bool Recipient::RemovesMember (ScriptTicket & script, int recipIdx)
{
	if (!script.HasDispatcherAddendum ())
		return false;
	ScriptInfo::RecipientInfo const & addressee = script.GetAddressee (recipIdx);

	return script.RemovesMember (addressee.GetHubId (), addressee.GetUserId ());
}

LocalRecipient::LocalRecipient (Address const & address,
								int projectId,
								FilePath const & inboxPath)
    : Recipient (address), 
	  _projectId (projectId),
	  _inboxPath (inboxPath)
{}

bool LocalRecipient::AcceptScript (ScriptTicket & script, MailTruck & truck, int recipIdx)
{
	if (script.IsStamped(recipIdx))
	{
		// Making sure DeliveredToAnyoneOnThisMachine is true
		script.MarkDeliveredToAnyone();
		return true;
	}
	dbg << "    LocalRecipient::AcceptScript" << std::endl;

	if (_projectId == -1) // special recipient awaiting project creation (invitation mechanism)
	{
		if (IsRemoved ()) // no longer active member
			script.StampDelivery (recipIdx);

		if (!script.IsInvitation ())
		{
			// a script following invitation
			// can't be processed till invitee enlistment is created
			script.SetIsPostponed ();
		}
		return true;
	}

	if (IsRemoved ()) // no longer active member
	{
		// if this is the script that has removed this member, dispatch it anyway
		if (RemovesMember (script, recipIdx))
			truck.CopyLocal (script, recipIdx, _projectId, _inboxPath);
		else
			script.StampDelivery (recipIdx);
	}
	else
	{
		truck.CopyLocal (script, recipIdx, _projectId, _inboxPath);
	}
	return true;
}

// ==================
// LocalRecipientList
// ==================

// Active recipients are at the front, Removed recipients are at the back of the list

void LocalRecipientList::Add (LocalRecipient const & recip)
{
	Assert (!recip.IsRemoved ());
	_catalog.AddProjectMember (recip, recip.GetProjectId ());

	iterator existing = FindByAddress (recip);
	if (existing == end ())
	{
		_recips.push_front (recip);
	}
	else
	{
		if (existing->IsRemoved ())
		{
			_recips.erase (existing);
			_recips.push_front (recip);
		}
		else
			existing->SetProjectId (recip.GetProjectId ());
	}
}

void LocalRecipientList::AddRemoved (LocalRecipient const & recip)
{
	Assert (recip.IsRemoved ());
	_catalog.AddRemovedProjectMember (recip, recip.GetProjectId ());

	iterator existing = FindByAddress (recip);
	if (existing == end ())
	{
		_recips.push_back (recip);
	}
	else
	{
		if (existing->IsRemoved ())
		{
			existing->SetProjectId (recip.GetProjectId ());
		}
		else
		{
			std::string userInfo = existing->GetProjectName ();
			userInfo += ", ";
			userInfo += existing->GetUserId ();
			throw Win::InternalException ("Illegal address database operation: "
				"Not allowed to change the state of an active local recipient.",
				userInfo.c_str ());
		}
	}
}

// remember to clear workQueue before doing this

void LocalRecipientList::Refresh (std::string const & currentHubId)
{
	_recips.clear ();

	for (UserSeq userSeq (_catalog);
		 !userSeq.AtEnd (); 
		 userSeq.Advance ())
    {
		ProjectUserData projData;
		userSeq.FillInProjectData (projData);

		LocalRecipient newLocalRecipient (projData.GetAddress (),
										  projData.GetProjectId (),
										  projData.GetInboxDir ());
		newLocalRecipient.MarkRemoved (userSeq.IsRemoved ());
		Insert (newLocalRecipient);
    }

	if (currentHubId.empty ())
		return;

	// sanity cleanup,
	// introduced as a part of a fix for issue #1140:
	// remove all active entries with hub id different than the current hub id for which
	// there exist parallel active entries having the same project name, user id, 
	// but with the current hub id
	// I cannot remove all entries with hub id varrying from current hub id, because
	// of backward compatibility with the 3.5 version. Users of 3.5 could select arbitrary
	// email addresses for project members. If someone converts from 3.5 straight to 4.5 or higher
	// I must give him time to start using unified hub id.
	std::vector<Address> toBeRemoved;
	for (const_iterator user = begin (); user != end (); ++user)
	{
		if (user->IsRemoved ())
			break;
		
		if (!IsNocaseEqual (currentHubId, user->GetHubId ()))
		{
			Address expectedAddr (*user);
			expectedAddr.SetHubId (currentHubId);
			const_iterator expectedUser = FindByAddress (expectedAddr);
			if (expectedUser != end () && !expectedUser->IsRemoved ())
			{
				toBeRemoved.push_back (*user);
			}
		}
	}

	for (unsigned int i = 0; i < toBeRemoved.size (); ++i)
	{
		// 1. change state to "removed" in catalog and
		_catalog.RemoveLocalUser (toBeRemoved [i]);
		// 2. move removed recipient to back on _recips list,
		// because order matters: "removed" are at the back
		iterator recip = FindByAddress (toBeRemoved [i]);
		Assert (recip != end ());
		Assert (!recip->IsRemoved ());

		LocalRecipient moved (*recip);
		moved.MarkRemoved (true);

		_recips.erase (recip);
		_recips.push_back (moved);
	}
}

void LocalRecipientList::GetActiveProjectList (NocaseSet & projects) const
{
	for (const_iterator en = begin (); en != end (); ++en)
	{
		if (en->IsRemoved ())
			break;
		
		if (!en->HasRandomUserId ())
			projects.insert (en->GetProjectName ());
	}
}

void LocalRecipientList::Clear () 
{
	_recips.clear ();
}

void LocalRecipientList::Insert (LocalRecipient const & recip) 
{
	if (recip.IsRemoved ())
		_recips.push_back (recip);
	else
		_recips.push_front (recip);
}

void LocalRecipientList::HubIdChanged (std::string const & oldHubId, std::string const & newHubId)
{
	_catalog.LocalHubIdChanged (oldHubId, newHubId);
	Refresh ();
}

void LocalRecipientList::KillRecipient (Address const & address, int projectId)
{
	for (iterator en = begin (); en != end (); ++en)
	{
		if (address.IsEqualAddress (*en) &&
			(projectId == en->GetProjectId ()))
		{
			_catalog.KillLocalRecipient (address, projectId);
			_recips.erase (en);
			break;
		}
	}
}

// Returns false if there exists an active local member
// with the given address.
// In that case the recipient is not removed from database.
bool LocalRecipientList::KillRemovedRecipient (Address const & address)
{
	iterator recip = FindByAddress (address);
	if (recip != end ())
	{
		if (recip->IsRemoved ())
		{
			_catalog.KillRemovedLocalRecipient (address);
			_recips.erase (recip);
		}
		else
			return false;
	}
	return true;
}

void LocalRecipientList::PurgeInMemory ()
{
	iterator firstRemoved = _recips.begin ();
	while (firstRemoved != _recips.end () && !firstRemoved->IsRemoved ())
		++firstRemoved;

	// Cannot use resize, because LocalRecipient has no default constructor
	// _recips.resize (newSize);
	_recips.erase (firstRemoved, _recips.end ());
}

LocalRecipient * LocalRecipientList::Find (Address const & address)
{
	iterator recip = FindByAddress (address);
	return recip == end () ? 0 : &*recip;
}

Recipient * LocalRecipientList::FindWildcard (std::string const & hubId, 
										   std::string const & projName, 
										   std::string const & senderHubId, 
										   std::string const & senderUserId)
{
	for (iterator en = begin (); en != end (); ++en)
	{
		if (en->IsRemoved ())
			break;

		if (en->HasEqualProjectName (projName) &&
			en->HasEqualHubId (hubId) &&
			!en->HasRandomUserId ())
		{
			return &(*en);
		}
	}
	return 0;
}

bool LocalRecipientList::FindProject (std::string const & projName) const
{
	for (const_iterator en = begin (); en != end (); ++en)
	{
		if (en->HasEqualProjectName (projName))
		{
			return true;
		}
	}
	return false;
}

bool LocalRecipientList::FindActiveProject (std::string const & projName) const
{
	for (const_iterator en = begin (); en != end (); ++en)
	{
		if (en->IsRemoved ())
			return false;

		if (en->HasEqualProjectName (projName))
		{
			return true;
		}
	}
	return false;
}

// Ignore hubId
LocalRecipient * LocalRecipientList::FindRelaxed (Address const & address)
{
	iterator result = std::find_if (begin (), 
									end (), 
									Recipient::HasEqualProjectUser (address));
	
	return result == end () ? 0 : &*result;
}

bool LocalRecipientList::EnlistmentAwaitsFullSync (std::string const & projectName) const
{
	for (const_iterator seq = begin (); seq != end (); ++seq)
	{
		if (seq->IsRemoved ())
			break;

		if (IsNocaseEqual (projectName, seq->GetProjectName ()))
		{
			// Project with the same name
			if (seq->HasRandomUserId ())
				return true;	// Project awaits a full sync script
		}
	}
	return false;
}

void LocalRecipientList::UpdateAddress (
		Address const & currentAddress,
		std::string const & newHubId, 
		std::string const & newUserId)
{
	// first add then remove (in case we're interrupted)
	Assert (!newHubId.empty ());
	Assert (!newUserId.empty ());

	_catalog.RefreshUserData (currentAddress, newHubId, newUserId);

	iterator oldEntry = FindByAddress (currentAddress);
	Assert (oldEntry != _recips.end ());
	if (oldEntry->IsRemoved ())
		return;

	int const projId = oldEntry->GetProjectId ();
	FilePath const inbox (oldEntry->GetInbox ());
	Address newAddress (newHubId, currentAddress.GetProjectName (), newUserId);

	iterator newEntry = FindByAddress (newAddress);
	if (newEntry == _recips.end ())
	{
		LocalRecipient newRecip (newAddress, projId, inbox);
		_recips.push_front (newRecip);
	}
	else
	{
		if (newEntry->IsRemoved ())
		{
			_recips.erase (newEntry);
			LocalRecipient newRecip (newAddress, projId, inbox);
			_recips.push_front (newRecip);
		}
	}

	if (!IsNocaseEqual (newHubId, currentAddress.GetHubId ()))
	{
		// change of hub id:
		// keep the old entry as "removed" in a case 
		// scripts with the old address arrive later
		LocalRecipient oldRecip (currentAddress, projId, inbox);
		oldRecip.MarkRemoved (true);
		_recips.push_back (oldRecip);
	}
	
	_recips.erase (FindByAddress (currentAddress));
}

LocalRecipientList::iterator LocalRecipientList::FindByAddress (Address const & address)
{
	return find_if (begin (), end (), Recipient::HasEqualAddress (address));
}
