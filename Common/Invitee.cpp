// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "Invitee.h"

void Invitee::Serialize (Serializer& out) const
{
	Address::Serialize (out);
	_user.Serialize (out);
	_computer.Serialize (out);
	out.PutBool (_isObserver);
}

void Invitee::Deserialize (Deserializer& in, int version)
{
	Address::Deserialize (in, version);
	_user.Deserialize (in, version);
	_computer.Deserialize (in, version);
	_isObserver = in.GetBool ();
}
