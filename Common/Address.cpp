//----------------------------------
// (c) Reliable Software 2002 - 2006
//----------------------------------
#include "precompiled.h"
#include "Address.h"
#include "UserIdPack.h"
#include <StringOp.h>

bool Address::operator < (Address const & add) const
{
	if (IsNocaseLess (_projName, add.GetProjectName ()))
		return true;
	if (IsNocaseEqual (_projName, add.GetProjectName ()))
	{
		if (IsNocaseLess (_hubId, add.GetHubId ()))
			return true;
		if (IsNocaseEqual (_hubId, add.GetHubId ()))
			return IsNocaseLess (_userId, add.GetUserId ());
	}
	return false;
}

bool Address::IsEqualAddress (Address const & address) const
{
	return IsNocaseEqual (_projName, address.GetProjectName ()) &&
		   IsNocaseEqual (_hubId, address.GetHubId ()) &&
		   IsNocaseEqual (_userId, address.GetUserId ());
}

void Address::Serialize (Serializer& out) const
{
	_projName.Serialize (out);
	_hubId.Serialize (out);
	_userId.Serialize (out);
}

void Address::Deserialize (Deserializer& in, int version)
{
	if (version > 2)
		_projName.Deserialize (in, version);
	_hubId.Deserialize (in, version);
	_userId.Deserialize (in, version);
}

void Address::Dump (std::ostream & out) const
{
	out << _projName << " | " << _hubId << " | id-" << UserIdPack (_userId).GetUserIdString ();
}


 