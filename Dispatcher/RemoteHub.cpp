// ----------------------------------
// (c) Reliable Software, 2002 - 2006
// ----------------------------------
#include "precompiled.h"
#include "RemoteHub.h"
#include "Catalog.h"
#include "HubTransportDlg.h"
#include "Prompter.h"
#include "resource.h"

RemoteHubList::RemoteHubList (Catalog & catalog)
: _catalog (catalog)
{
	for (HubListSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		std::string hubId;
		Transport transport;
		seq.GetHubEntry (hubId, transport);
		_hubMap [hubId] = transport;
	}
}

bool RemoteHubList::IsKnown (std::string const & hubId) const
{
	Iterator existing = _hubMap.find (hubId);
	return existing != _hubMap.end ();
}

Transport const & RemoteHubList::GetInterClusterTransport (std::string const & hubId) const
{
	Iterator it = _hubMap.find (hubId);
	Assert (it != _hubMap.end ());
	return it->second;
}

void RemoteHubList::GetAskTransport (std::string const & hubId, Transport & transport)
{
	Iterator it = _hubMap.find (hubId);
	// common case
	if (it != _hubMap.end ())
	{
		if (!it->second.IsUnknown ())
		{
			transport = it->second;
			return;
		}
	}
	// not found or found unknown (should only happen during testing)
	// maybe the hubId encodes transport
	transport.Init (hubId);
	if (transport.IsUnknown ())
	{
		HubTransportCtrl ctrl (hubId);
		if (ThePrompter.GetData (ctrl, "Couldn't send scripts!\n"
									   "Transport to a remote hub is unknown."))
		{
			transport = ctrl.GetTransport ();
			Assert (!transport.IsUnknown ());
		}
		else
		{
			transport = Transport ();
			return;
		}
	}
	_hubMap [hubId] = transport;
	// checks for duplicates
	_catalog.AddRemoteHub (hubId, transport);
}

bool RemoteHubList::VerifyAdd (std::string const & hubId)
{
	Iterator it = _hubMap.find (hubId);
	// common case
	if (it != _hubMap.end ())
	{
		if (!it->second.IsUnknown ())
		{
			return true;
		}
	}
	// maybe the hubId encodes transport
	Transport transport (hubId);
	if (!transport.IsUnknown ())
	{
		_hubMap [hubId] = transport;
		// checks for duplicates
		_catalog.AddRemoteHub (hubId, transport);
		return true;
	}
	return false;
}

void RemoteHubList::Add (std::string const & hubId, Transport const & transport)
{
	if (!transport.IsUnknown ())
	{
		_hubMap [hubId] = transport;
		_catalog.AddRemoteHub (hubId, transport);
	}
}

void RemoteHubList::AddReplaceHubId (std::string const & oldHubId, 
									 std::string const & newHubId,
									 Transport const & newTransport)
{
	Assert (!IsNocaseEqual (oldHubId, newHubId));

	if (newHubId.empty ())
		return;

	Transport transport (newTransport);
	if (transport.IsUnknown ())
	{
		Iterator it = _hubMap.find (oldHubId);
		if (it != _hubMap.end ())
		{
			transport = it->second;
		}
		else
		{
			transport.Init (newHubId);
		}
	}

	if (!transport.IsUnknown ())
	{
		_hubMap [newHubId] = transport;
		_catalog.AddRemoteHub (newHubId, transport);
	}
}

void RemoteHubList::Delete (std::string const & hubId)
{
	NocaseMap<Transport>::iterator it = _hubMap.find (hubId);
	if (it != _hubMap.end ())
	{
		_catalog.DeleteRemoteHub (hubId);
		_hubMap.erase (it);
	}
}

void RemoteHubList::UpdateInterClusterTransport (std::string const & hubId,
											  Transport const & newTransport)
{
	_catalog.AddRemoteHub (hubId, newTransport);
	_hubMap [hubId] = newTransport;
}

// Table interface
void RemoteHubList::QueryUniqueNames (std::vector<std::string> & unames, Restriction const * restrict)
{
	Assert (restrict == 0);
	unames.reserve (_hubMap.size ());
	for (Iterator it = _hubMap.begin (); it != _hubMap.end (); ++it)
	{
		unames.push_back (it->first);
	}
}

int	RemoteHubList::GetNumericField (Column col, std::string const & uname) const
{
	Assert (col == colMethod);
	Iterator it = _hubMap.find (uname);
	Assert (it != _hubMap.end ());
	return it->second.GetMethod ();
}

std::string	RemoteHubList::GetStringField (Column col, std::string const & uname) const
{
	Assert (col == colRoute);
	Iterator it = _hubMap.find (uname);
	Assert (it != _hubMap.end ());
	return it->second.GetRoute (); 
}
