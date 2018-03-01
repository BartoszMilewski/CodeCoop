// ----------------------------------
// (c) Reliable Software, 1999 - 2006
// ----------------------------------
#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING true
#include "ClusterRecipient.h"
#include "Catalog.h"
#include "MailTruck.h"
#include "ScriptInfo.h"
#include "UserIdPack.h"

bool ClusterRecipient::AcceptScript (ScriptTicket & script, MailTruck & truck, int recipIdx)
{
	if (script.IsStamped(recipIdx))
		return true;
	dbg << "    ClusterRecipient::AcceptScript" << std::endl;
	if (IsRemoved ())  // no longer active member
	{
		// if this is the script that removed this member, dispatch it anyway
		if (RemovesMember (script, recipIdx))
			script.AddRequest(recipIdx, _transport);
		else
			script.StampDelivery (recipIdx);
		return true;
	}
	else if (_transport.IsUnknown ())
	{
		// Don't know where to forward
		return false;
	}
	else
	{
		script.AddRequest(recipIdx, _transport);
		return true;
	}
}

//-----------------------
// Cluster Recipient List
//-----------------------

unsigned ClusterRecipientList::Refresh ()
{
	_recips.clear ();
	ClusterRecipient recip;
	unsigned count = 0;
	for (ClusterRecipSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		seq.GetClusterRecipient (recip);
		if (recip.IsRemoved ())
			_recips.push_back (recip);
		else
		{
			_recips.push_front (recip);
			++count;
		}
	}
	return count;
}

ClusterRecipient * ClusterRecipientList::Add (ClusterRecipient const & newRecip)
{
	Assert (!newRecip.IsRemoved ());
	_catalog.AddClusterRecipient (newRecip, newRecip.GetTransport ());
	
	iterator result = FindByAddress (newRecip);
	if (end () == result)
	{
		_recips.push_front (newRecip);
		return &_recips.front ();
	}
	else
	{
		if (result->IsRemoved ())
		{
			_recips.erase (result);
			_recips.push_front (newRecip);
			return &_recips.front ();
		}
		else
		{
			if (newRecip.GetTransport () != result->GetTransport ())
			{
				result->ChangeTransport (newRecip.GetTransport ());
			}
			return &(*result);
		}
	}
}

void ClusterRecipientList::AddRemoved (ClusterRecipient const & newRecip)
{
	Assert (newRecip.IsRemoved ());
	_catalog.AddRemovedClusterRecipient (newRecip, newRecip.GetTransport ());

	iterator existing = FindByAddress (newRecip);
	if (existing == end ())
	{
		_recips.push_back (newRecip);
	}
	else
	{
		existing->MarkRemoved(true);
		existing->ChangeTransport (newRecip.GetTransport ());
	}
}

void ClusterRecipientList::GetActiveSatelliteTransports (std::set<Transport> & transports) const
{
	// only well-defined transports of active members
	for (const_iterator it = _recips.begin (); it != _recips.end (); ++it)
	{
		if (it->IsRemoved ())
			break;
		
		if (!it->GetTransport ().IsUnknown ())
			transports.insert (it->GetTransport ());
	}
}

void ClusterRecipientList::GetActiveProjectList (NocaseSet & projects) const
{
	for (const_iterator it = _recips.begin (); it != _recips.end (); ++it)
	{
		if (it->IsRemoved ())
			break;
		
		if (!it->HasRandomUserId ())
			projects.insert (it->GetProjectName ());
	}
}

void ClusterRecipientList::GetActiveOffsiteSatList (NocaseSet & addresses) const
{
	for (const_iterator it = _recips.begin (); it != _recips.end (); ++it)
	{
		if (it->IsRemoved ())
			break;
		
		Transport const & tr = it->GetTransport ();
		if (tr.IsEmail ())
			addresses.insert (tr.GetRoute ());
	}
}

void ClusterRecipientList::RemoveRecipient (Address const & address)
{
	_catalog.RemoveClusterRecipient (address);
	iterator it = FindByAddress (address);
	if (it != _recips.end ())
	{
		if (!it->IsRemoved ())
		{
			ClusterRecipient newRecip (address, it->GetTransport ());
			_recips.erase (it);
			_recips.push_back (newRecip);
		}
	}
}

void ClusterRecipientList::KillRecipient (Address const & address)
{
	_catalog.DeleteClusterRecipient (address);
	iterator it = FindByAddress (address);
	if (it != _recips.end ())
		_recips.erase (it);
}

ClusterRecipient * ClusterRecipientList::Find (Address const & address)
{
    iterator result = FindByAddress (address);
	return result == end () ? 0 : &(*result);
}

ClusterRecipient const * ClusterRecipientList::Find (Address const & address) const
{
	const_iterator result = FindByAddress (address);
	return result == end () ? 0 : &(*result);
}

// Ignore hubId
ClusterRecipient * ClusterRecipientList::FindRelaxed (Address const & address)
{
	iterator result = std::find_if (begin (), 
									end (), 
									Recipient::HasEqualProjectUser (address));
	
	return result == end () ? 0 : &*result;
}


Recipient * ClusterRecipientList::FindWildcard (std::string const & hubId, 
										   std::string const & projName, 
										   std::string const & senderHubId, 
										   std::string const & senderUserId)
{
	Recipient * recipient = 0;
	// Heuristics: find user with lowest id
	for (iterator clu = begin (); clu != end (); ++clu)
	{
		if (clu->HasEqualProjectName (projName) &&
			clu->HasEqualHubId (hubId) &&
			!clu->HasRandomUserId () &&
			!clu->IsRemoved ())
		{
			if (recipient != 0)
			{
				UserIdPack uidLast (recipient->GetUserId ());
				UserIdPack curUid (clu->GetUserId ());
				if (curUid.GetUserId () < uidLast.GetUserId ())
					recipient = &(*clu);
			}
			else
				recipient = &(*clu);
		}
	}
	return recipient;
}

std::string ClusterRecipientList::FindPath (std::string const & computerName) const
{
	for (const_iterator it = begin (); it != end (); ++it)
	{
		if (it->IsRemoved ())
			break;

		Transport const & transport = it->GetTransport ();
		if (transport.IsNetwork ())
		{
			std::string const & netPath = transport.GetRoute ();
			FullPathSeq seq (netPath.c_str ());
			if (seq.IsUNC () &&	IsNocaseEqual (computerName, seq.GetServerName ()))
				return netPath;
		}
	}
	return std::string ();
}

int ClusterRecipientList::CountTransportUsers (Transport const & transport) const
{
	int count = 0;
	for (const_iterator it = begin (); it != end (); ++it)
	{
		if (it->IsRemoved ())
			break;
		
		if (it->GetTransport () == transport)
			++count;
	}
	return count;
}

void ClusterRecipientList::UpdateAddress (
		Address const & currentAddress,
		std::string const & newHubId, 
		std::string const & newUserId)
{
    // first add then remove (in case we're interrupted)
	Assert (!newHubId.empty ());
	Assert (!newUserId.empty ());

	iterator oldEntry = FindByAddress (currentAddress);
	Assert (oldEntry != _recips.end ());

	Transport const oldTransport (oldEntry->GetTransport ());
	Address newAddress (newHubId, currentAddress.GetProjectName (), newUserId);
	
	_catalog.AddClusterRecipient (newAddress, oldTransport);

	// add recipient with New address, but with old recipient's path
	iterator result = FindByAddress (newAddress);
	if (result == end ())
	{
		ClusterRecipient newRecip (newAddress, oldTransport);
		_recips.push_front (newRecip);
	}
	else
	{
		if (result->IsRemoved ())
		{
			_recips.erase (result);
			ClusterRecipient newRecip (newAddress, oldTransport);
			_recips.push_front (newRecip);
		}
	}

	if (IsNocaseEqual (currentAddress.GetHubId (), newHubId))
	{
		_catalog.DeleteClusterRecipient (currentAddress);
	}
	else
	{
		// change of hub id:
		// keep the old entry as "removed" in a case 
		// scripts with the old address arrive later
		_catalog.RemoveClusterRecipient (currentAddress);
		ClusterRecipient oldRecipient (currentAddress, oldTransport);
		oldRecipient.MarkRemoved (true);
		_recips.push_back (oldRecipient);
	}

	_recips.erase (FindByAddress (currentAddress));
}

void ClusterRecipientList::UpdateTransport (Address const & address, 
											Transport const & transport)
{
    iterator result = FindByAddress (address);
	Assert (result != end ());
	_catalog.ChangeTransport (address, transport);
	result->ChangeTransport (transport);
	// activate the recip and put it in front of the list
	ClusterRecipient newRecip (*result);
	newRecip.MarkRemoved (false);
	_recips.erase (result);
	_recips.push_front (newRecip);
}

void ClusterRecipientList::ReplaceTransport (
			Transport const & oldTransport, 
			Transport const & newTransport)
{
	if (oldTransport == newTransport)
		return;

	for (iterator it = _recips.begin (); it != _recips.end (); ++it)
	{
		if (it->IsRemoved ())
			continue;

		if (oldTransport == it->GetTransport ())
		{
			_catalog.ChangeTransport (*it, newTransport);
			it->ChangeTransport (newTransport);
		}
	}
}

void ClusterRecipientList::Activate (Address const & address)
{
	iterator oldEntry = FindByAddress (address);
	Assert (oldEntry != _recips.end ());
	Assert (oldEntry->IsRemoved ());
	_catalog.ActivateClusterRecipient (address);
	ClusterRecipient newRecip (*oldEntry);
	newRecip.MarkRemoved (false);
	_recips.erase (oldEntry);
	_recips.push_front (newRecip);
}

void ClusterRecipientList::ClearPersistent ()
{
	_catalog.ClearClusterRecipients ();
	_recips.clear ();
}

void ClusterRecipientList::PurgeInMemory ()
{
	unsigned int newSize = 0;
	for (const_iterator it = _recips.begin (); 
		 it != _recips.end () && !it->IsRemoved (); 
		 ++it)
	{
		newSize++;
	}
	_recips.resize (newSize);
}

ClusterRecipientList::iterator ClusterRecipientList::FindByAddress (Address const & address)
{
	return find_if (begin (), end (), Recipient::HasEqualAddress (address));
}

ClusterRecipientList::const_iterator ClusterRecipientList::FindByAddress (Address const & address) const
{
	return find_if (begin (), end (), Recipient::HasEqualAddress (address));
}
