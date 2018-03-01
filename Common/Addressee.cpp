//----------------------------------
// (c) Reliable Software 1998 - 2005
// ---------------------------------

#include "precompiled.h"

#include "Addressee.h"
#include "Address.h"

#include <Dbg/Assert.h>
#include <StringOp.h>

Addressee::Addressee (Addressee const & addressee)
{
	_hubId = addressee._hubId;
	_userId = addressee._userId;
	_receivedScript = addressee._receivedScript;
}

std::string const & Addressee::GetDisplayUserId () const 
{
	static const std::string admin ("\"project administrator\"");
	if (HasWildcardUserId ())
	{
		return admin;
	}
	return _userId;
}

bool Addressee::HasWildcardUserId () const
{
	return _userId.compare ("*") == 0;
}

bool Addressee::IsHubDispatcher () const 
{
	return GetStringUserId () == DispatcherAtHubId;
}

bool Addressee::IsSatDispatcher () const 
{
	return GetStringUserId () == DispatcherAtSatId;
}

bool Addressee::IsEqual (std::string const & hubId, std::string const & userId) const
{
	if (_hubId.empty ())
	{
		if (!hubId.empty ())
			return false;
	}
	else
	{
		if (hubId.empty ())
			return false;
		if (!IsNocaseEqual (_hubId, hubId))
			return false;
	}
	// equal hubId
	if (_userId.empty ())
	{
		if (!userId.empty ())
			return false;
	}
	else
	{
		if (userId.empty ())
			return false;
		if (!IsNocaseEqual (_userId, userId))
			return false;
	}
	return true;
}

void Addressee::Serialize (Serializer& out) const
{
    _hubId.Serialize (out);
    _userId.Serialize (out);
}

void Addressee::Deserialize (Deserializer& in, int version)
{
    _hubId.Deserialize (in, version);
	_hubId.Trim ();
    _userId.Deserialize (in, version);
	_userId.Trim ();
}

std::ostream& operator<<(std::ostream& os, Addressee const & a)
{
	os << a.GetHubId () << " (" << a.GetDisplayUserId () << "), " << (a.ReceivedScript () ? "delivered" : "NOT delivered");
	return os;
}

std::ostream& operator<<(std::ostream& os, AddresseeList const & addrList)
{
	for (AddresseeList::ConstIterator it = addrList.begin(); it != addrList.end(); ++it)
		os << "    " << *it << std::endl;
	return os;
}

