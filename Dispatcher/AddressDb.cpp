// ----------------------------------
// (c) Reliable Software, 1999 - 2006
// ----------------------------------
#include "precompiled.h"
#include "AddressDb.h"
#include "Catalog.h"
#include "RecipKey.h"
#include "UserIdPack.h"
#include "AlertMan.h"
#include "ScriptInfo.h"
#include <StringOp.h>

bool AddressDatabase::EmptyRecipient::AcceptScript (ScriptTicket * script, MailTruck & truck, int recipIdx)
{
	script->StampDelivery (recipIdx);
	return true;
}

AddressDatabase::AddressDatabase (Catalog & catalog, std::string const & hubId, bool isLoadCluster)
	: _catalog (catalog),
	  _clusterRecipientList (catalog),
	  _localRecipientList (catalog),
	  _remoteHubList (catalog)
{
	LoadLocalRecips (hubId);
    if (isLoadCluster)
	{
		LoadClusterRecips ();
	}
}

unsigned AddressDatabase::LoadClusterRecips ()
{
	return _clusterRecipientList.Refresh ();
}

void AddressDatabase::LoadLocalRecips (std::string const & currentHubId)
{
	_localRecipientList.Refresh (currentHubId);
}

void AddressDatabase::GetInviteeList (std::vector<Address> & inviteeList) const
{
	inviteeList.clear ();
	for (LocalRecipientList::const_iterator it = _localRecipientList.begin ();
		 it != _localRecipientList.end ();
		 ++it)
	{
		if (it->IsRemoved ())
			break;

		if (it->GetProjectId () == -1)
			inviteeList.push_back (*it);
	}
}

bool AddressDatabase::VerifyAddRemoteHub (std::string const & hubId)
{
	return _remoteHubList.VerifyAdd (hubId);
}

void AddressDatabase::GetAskRemoteTransport (std::string const & hubId, Transport & transport)
{
	_remoteHubList.GetAskTransport (hubId, transport);
}

void AddressDatabase::ChangeRemoteHubId (std::string const & oldHubId, 
										 std::string const & newHubId, 
										 Transport const & newTransport)
{
	_remoteHubList.AddReplaceHubId (oldHubId, newHubId, newTransport);
}

void AddressDatabase::DeleteRemoteHub (std::string const & hubId)
{
	_remoteHubList.Delete (hubId);
}

void OrderByImportance (NocaseMap<int> & idMap, std::vector<std::string> & idList)
{
	while (!idMap.empty ())
	{
		NocaseMap<int>::iterator maxElem = idMap.begin ();
		NocaseMap<int>::iterator it = maxElem;
		++it;
		while (it != idMap.end ())
		{
			if (it->second > maxElem->second)
				maxElem = it;
			++it;
		}
		idList.push_back (maxElem->first);
		idMap.erase (maxElem);
	}
}

void AddressDatabase::ListLocalHubIdsByImportance (std::vector<std::string> & idList) const
{
	NocaseMap<int> idMap;
	for (LocalRecipientList::const_iterator it = _localRecipientList.begin ();
		 it != _localRecipientList.end ();
		 ++it)
	{
		if (!it->IsRemoved ())
			++ idMap [it->GetHubId ()];
	}
	OrderByImportance (idMap, idList);
}

void AddressDatabase::ListClusterHubIdsByImportance (std::vector<std::string> & idList) const
{
	NocaseMap<int> idMap;
	for (ClusterRecipientList::const_iterator it = _clusterRecipientList.begin ();
		 it != _clusterRecipientList.end ();
		 ++it)
	{
		if (!it->IsRemoved ())
			++idMap [it->GetHubId ()];
	}
	OrderByImportance (idMap, idList);
}

Recipient * AddressDatabase::Find (Address const & address)
{
	if (_nobody.HasEqualHubId (address.GetHubId ()))
		return &_nobody;
	
	// In order to deal correctly with address changes (hubId changes)
	// we relax the search by ignoring the hubId

	Recipient * localCandidate = _localRecipientList.FindRelaxed (address);
	if (localCandidate != 0 && !localCandidate->IsRemoved ())
		return localCandidate;	// Active local recipient

	Assert (localCandidate == 0 || localCandidate->IsRemoved ());

	Recipient * clusterCandidate = _clusterRecipientList.FindRelaxed (address);
	if (clusterCandidate != 0 && !clusterCandidate->IsRemoved ())
		return clusterCandidate;// Active cluster recipient

	Assert (clusterCandidate == 0 || clusterCandidate->IsRemoved ());

	if (localCandidate == 0)
		return clusterCandidate;

	if (clusterCandidate == 0)
		return localCandidate;

	Assert ((localCandidate != 0 && localCandidate->IsRemoved ()) &&
		    (clusterCandidate != 0 && clusterCandidate->IsRemoved ()));

	// From both removed recipients favor local recipient
	return localCandidate;
}

Recipient * AddressDatabase::FindLocal (Address const & address)
{
	return _localRecipientList.Find (address);
}

// Find recipient who:
// 1. has given hubId and project name
// 2. has arbitrary (but not random!) userId
// 3. differs from sender
// 4. preferably has his enlistment on this machine
Recipient * AddressDatabase::FindWildcard (std::string const & hubId, 
										   std::string const & projName, 
										   std::string const & senderHubId, 
										   std::string const & senderUserId)
{
	if (_nobody.HasEqualHubId (hubId))
		return &_nobody;

	Recipient * result = _localRecipientList.FindWildcard (hubId, projName, senderHubId, senderUserId);
	if (result == 0)
		result = _clusterRecipientList.FindWildcard (hubId, projName, senderHubId, senderUserId);
	return result;
}

std::string AddressDatabase::FindSatellitePath (std::string const & computerName) const
{
	return _clusterRecipientList.FindPath (computerName);
}

int AddressDatabase::CountTransportUsers (Transport const & transport) const
{
	return _clusterRecipientList.CountTransportUsers (transport);
}

// Returns false if there exists an active local member
// with the given address.
// In that case the recipient is not removed from database.
bool AddressDatabase::KillRemovedLocalRecipient (Address const & address)
{
	return _localRecipientList.KillRemovedRecipient (address);
}

void AddressDatabase::AddTempLocalRecipient (Address const & address, bool isRemoved)
{
	LocalRecipient invitedRecip (address, -1, FilePath ());
	invitedRecip.MarkRemoved (isRemoved);
	if (isRemoved)
	{
		_localRecipientList.AddRemoved (invitedRecip);
	}
	else
	{
		_localRecipientList.Add (invitedRecip);
	}
}

void AddressDatabase::KillTempLocalRecipient (Address const & address)
{
	_localRecipientList.KillRecipient (address, -1);
}

Recipient * AddressDatabase::AddClusterRecipient (Address const & address, Transport const & transport)
{
	return _clusterRecipientList.Add (ClusterRecipient (address, transport));
}

void AddressDatabase::AddRemovedClusterRecipient (Address const & address)
{
	ClusterRecipient recip (address, Transport ());
	recip.MarkRemoved (true);
	_clusterRecipientList.AddRemoved (recip);
}

ClusterRecipient const * AddressDatabase::FindClusterRecipient (Address const & address) const
{
	return _clusterRecipientList.Find (address);
}

void AddressDatabase::KillClusterRecipient (Address const & address)
{
	_clusterRecipientList.KillRecipient (address);
}

void AddressDatabase::ForgetClusterRecips ()
{
    _clusterRecipientList.ClearPersistent ();
}

// Return true if any addresses removed
bool AddressDatabase::Purge (bool purgeLocal, bool purgeSat)
{
	bool anyRemoved = _catalog.PurgeRecipients (purgeLocal, purgeSat);
	if (purgeLocal)
		_localRecipientList.PurgeInMemory ();
	if (purgeSat)
		_clusterRecipientList.PurgeInMemory ();

	return anyRemoved;
}

// Returns false if new address is already taken by a local recipient
bool AddressDatabase::UpdateAddress (std::string const & project, 
         std::string const & oldHubId, std::string const & oldUserId,
         std::string const & newHubId, std::string const & newUserId)
{
	Assert (!project.empty ());
	Assert (!oldHubId.empty ());
	Assert (!oldUserId.empty ());
	Assert (!newHubId.empty ());
	Assert (!newUserId.empty ());

	if (IsNocaseEqual (oldHubId, newHubId) && IsNocaseEqual (oldUserId, newUserId))
		return true;

	LocalRecipient * localRecip = _localRecipientList.Find (Address (oldHubId, project, oldUserId));
	if (localRecip != 0)
	{
		_clusterRecipientList.KillRecipient (Address (newHubId, project, newUserId));
		_localRecipientList.UpdateAddress (*localRecip, newHubId, newUserId);
	}
	else
	{
		ClusterRecipient * clusterRecip = _clusterRecipientList.Find (Address (oldHubId, project, oldUserId));
		if (clusterRecip != 0)
		{
			if (!KillRemovedLocalOrWarnUser (Address (newHubId, project, newUserId)))
				return false;

			_clusterRecipientList.UpdateAddress (*clusterRecip, newHubId, newUserId); 
		}
	}
	return true;
}

void AddressDatabase::RemoveAddress (Address const & address)
{
	// _localRecipientList -- nothing to do. Dispatcher does not remove from this list
	_clusterRecipientList.RemoveRecipient (address); 
}

void AddressDatabase::RemoveSatelliteAddress (Address const & address)
{
	_clusterRecipientList.RemoveRecipient (address); 
}

void AddressDatabase::HubIdChanged (std::string const & oldHubId, std::string const & newHubId)
{
	_localRecipientList.HubIdChanged (oldHubId, newHubId);
}

// Returns false if new address is already taken by a local recipient
bool AddressDatabase::UpdateTransport (Address const & address, Transport const & transport)
{
	ClusterRecipient * recip = _clusterRecipientList.Find (address);
	
    if (recip != 0)
	{
		if (recip->GetTransport () == transport)
		{
			if (recip->IsRemoved ())
				_clusterRecipientList.Activate (*recip);
		}
		else
			_clusterRecipientList.UpdateTransport (address, transport);
	}
	else
	{
		if (!KillRemovedLocalOrWarnUser (address))
			return false;

		_clusterRecipientList.Add (ClusterRecipient (address, transport));
    }
	return true;
}

void AddressDatabase::ReplaceTransport (
			Transport const & oldTransport, 
			Transport const & newTransport)
{
	_clusterRecipientList.ReplaceTransport (oldTransport, newTransport);
}

Transport const & AddressDatabase::GetInterClusterTransport (std::string const & hubId) const
{
	return _remoteHubList.GetInterClusterTransport (hubId);
}

void AddressDatabase::UpdateInterClusterTransport (std::string const & hubId,
												Transport const & newTransport)
{
	 _remoteHubList.UpdateInterClusterTransport (hubId, newTransport);
}

void AddressDatabase::AddInterClusterTransport (std::string const & hubId, Transport const & transport)
{
	_remoteHubList.Add (hubId, transport);
}

// returns false if local recipient with given address exists
bool AddressDatabase::KillRemovedLocalOrWarnUser (Address const & address)
{
	if (!_localRecipientList.KillRemovedRecipient (address))
	{
		TheAlertMan.PostInfoAlert ("Duplicate project member announcement from satellite.");
		return false;
	}
	return true;
}

void AddressDatabase::QueryUniqueNames (std::vector<std::string> & unames, 
										Restriction const * restrict)
{
	Assert (0 == restrict);
	_restriction = 0;
	std::set<std::string, NocaseLess> projectNames;

	for (LocalRecipientList::iterator it = _localRecipientList.begin ();
		 it != _localRecipientList.end ();
		 ++it)
	{
		std::string const & projName = it->GetProjectName ();
		Assert (!projName.empty ());
		if (projName.empty ())
		{
			KillRemovedLocalRecipient (Address (it->GetHubId (), it->GetProjectName (), it->GetUserId ()));
			throw Win::Exception ("Minor error in Dispatcher's database corrected");
		}
		projectNames.insert (projName);
	}
	for (ClusterRecipientList::iterator it = _clusterRecipientList.begin ();
		 it != _clusterRecipientList.end ();
		 ++it)
	{
		std::string const & projName = it->GetProjectName ();
		projectNames.insert (projName);
	}
	unames.clear ();
	unames.reserve (projectNames.size ());
	std::copy (projectNames.begin (), projectNames.end (), std::back_inserter (unames));
}

int	AddressDatabase::GetNumericField (Column col, std::string const & uname) const
{
	Assert (0 == _restriction);
	Assert (colMembers == col);
	int counter = 0;
	for (LocalRecipientList::const_iterator it = _localRecipientList.begin ();
		 it != _localRecipientList.end ();
		 ++it)
	{
		if (IsNocaseEqual (uname, it->GetProjectName ()))
			++ counter;
	}
	for (ClusterRecipientList::const_iterator cit = _clusterRecipientList.begin ();
		 cit != _clusterRecipientList.end ();
		 ++cit)
	{
		if (IsNocaseEqual (uname, cit->GetProjectName ()))
			++ counter;
	}
	return counter;
}

// unique key for Project Member view consists of
// 1. hubId
// 2. userId
// 3. local/cluster flag (true/false values respectively)
void AddressDatabase::QueryUniqueTripleKeys (
			std::vector<TripleKey> & ukeys,
			Restriction const * restrict)
{
	Assert (restrict != 0);
	_restriction = restrict;
	Assert (_restriction->IsStringSet ());
	for (LocalRecipientList::iterator it = _localRecipientList.begin ();
		 it != _localRecipientList.end ();
		 ++it)
	{
		if (IsEqualNameUnderRestrict (*it))
		{
			ukeys.push_back (RecipientKey (it->GetHubId (), it->GetUserId (), true));
		}
	}
	for (ClusterRecipientList::iterator cit = _clusterRecipientList.begin ();
		 cit != _clusterRecipientList.end ();
		 ++cit)
	{
		if (IsEqualNameUnderRestrict (*cit))
		{
			ukeys.push_back (RecipientKey (cit->GetHubId (), cit->GetUserId (), false));
		}
	}
}

std::string	AddressDatabase::GetStringField (Column col, TripleKey const & ukey) const
{
	Assert (0 != _restriction);
	Assert (_restriction->IsStringSet ());

	switch (col)
	{
	case colPath:
	{
		RecipientKey const & recip = static_cast<RecipientKey const &>(ukey);
		if (recip.IsLocal ())
		{
			for (LocalRecipientList::const_iterator it = _localRecipientList.begin ();
				 it != _localRecipientList.end ();
				 ++it)
			{
				if (IsEqualAddressUnderRestrict (*it, recip))
				{
					return it->GetInbox ().GetDir ();
				}
			}
		}
		else
		{
			for (ClusterRecipientList::const_iterator cit = _clusterRecipientList.begin ();
				 cit != _clusterRecipientList.end ();
				 ++cit)
			{
				if (IsEqualAddressUnderRestrict (*cit, recip))
				{
					return cit->GetTransport ().GetRoute ();
				}
			}
		}
		break;
	}
	default:
		Assert (!"Invalid column requested");
	};
	return std::string ();
}

int	AddressDatabase::GetNumericField (Column col, TripleKey const & ukey) const
{
	Assert (_restriction != 0);
	Assert (colStatus == col);
	RecipientKey const & recip = static_cast<RecipientKey const &>(ukey);
	if (recip.IsLocal ())
	{
		for (LocalRecipientList::const_iterator it = _localRecipientList.begin ();
			 it != _localRecipientList.end ();
			 ++it)
		{
			if (IsEqualAddressUnderRestrict (*it, recip))
				return !it->IsRemoved ();
		}
	}
	else
	{
		for (ClusterRecipientList::const_iterator cit = _clusterRecipientList.begin ();
			 cit != _clusterRecipientList.end ();
			 ++cit)
		{
			if (IsEqualAddressUnderRestrict (*cit, recip))
				return !cit->IsRemoved ();
		}
	}
	return 0;
}

std::string AddressDatabase::QueryCaption (Restriction const & r) const
{
	std::string caption;
	if (r.IsStringSet ())
	{
		if (r.GetString ().empty ())
		{
			caption = "Satellite computers of this hub";
		}
		else
		{
			caption += "Local members/enlistments in project ";
			caption += r.GetString ();
		}
	}
	return caption;
}

bool AddressDatabase::IsEqualNameUnderRestrict (Recipient const & recip) const
{
	Assert (0 != _restriction);
	Assert (_restriction->IsStringSet ());

	return IsNocaseEqual (recip.GetProjectName (), _restriction->GetString ());
}

bool AddressDatabase::IsEqualAddressUnderRestrict (Recipient const & recip, 
												   RecipientKey const & ukey) const
{
	Assert (0 != _restriction);
	Assert (_restriction->IsStringSet ());

	return IsNocaseEqual (recip.GetProjectName (), _restriction->GetString ()) &&
		   recip.GetHubId () == ukey.GetHubId () &&
		   recip.GetUserId () == ukey.GetUserId ();
}

