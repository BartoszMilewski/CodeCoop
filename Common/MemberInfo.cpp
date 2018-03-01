//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "MemberInfo.h"
#include "OutputSink.h"
#include "ProjectDb.h"
#include "UserId.h"
#include "UserIdPack.h"
#include "License.h"

#include <XML/XmlTree.h>
#include <StringOp.h>

#include <iomanip>

MemberDescription const * MemberCache::GetMemberDescription (UserId userId)
{
	Assert (userId != gidInvalid);
	if (userId != _lastUserId)
	{
		_lastUserId = gidInvalid;
		_memberDescription = _dataBase.RetrieveMemberDescription (userId);
		if (_memberDescription.get () == 0)
			throw Win::Exception ("Corrupted Database: Member Info");
		_lastUserId = userId;
	}
	return _memberDescription.get ();
}

bool MemberState::operator < (MemberState const & state) const
{
	// Member state order: Defected < Receiver < Observer < Voting Member < Administrator
	// Active
	if (_bits._A != state._bits._A)
		return _bits._A == 0;

	// Receiver
	if (_bits._Recv != state._bits._Recv)
		return _bits._Recv == 1;

	// Voting
	if ( _bits._V != state._bits._V)
		return _bits._V == 0;

	// Admin
	if (_bits._Adm != state._bits._Adm)
		return _bits._Adm == 0;

	// All interesting bits equal, compare the rest
	return _value < state._value;
}

char const * MemberState::GetDisplayName () const
{
	if (IsActive ())
	{
		if (IsAdmin ())
		{
			if (IsDistributor ())
				return "Distribution Administrator";
			else
				return "Administrator";
		}
		if (IsVoting ())
		{
			if (IsDistributor ())
				return "Distribution Voting Member";
			else if (IsReceiver ())
				return "Receiver";
			else
				return "Voting Member";
		}
		else
			return "Observer";
	}
	else
	{
		return "Defected";
	}
}

std::ostream& operator<<(std::ostream& os, MemberState const & state)
{
	os << state.GetDisplayName ();
	os << "; _V:" << (state.IsVoting () ? '1' : '0'); 
	os << ", _A:" << (state.IsActive () ? '1' : '0'); 
	os << ", _Adm:" << (state.HasAdminPriv () ? '1' : '0'); 
	os << ", _S:" << (state.IsSuicide () ? '1' : '0'); 
	os << ", _Ver:" << (state.IsVerified () ? '1' : '0'); 
	os << ", _Distr:" << (state.IsDistributor () ? '1' : '0'); 
	os << ", _Recv:" << (state.IsReceiver () ? '1' : '0'); 
	os << ", _NoBra:" << (state.NoBranching () ? '1' : '0');
	os << ", _Out:" << (state.IsCheckoutNotification () ? '1' : '0');
	return os;
}

// Order, from most significant to least significant bits
// Active, Verified, Voting, Admin
bool MemberStateInfo::operator< (MemberStateInfo const & info) const
{
	MemberState state = info._state;
	if (_state.IsActive () && !state.IsActive ())
		return false;
	if (!_state.IsActive () && state.IsActive ())
		return true;
	if (_state.IsVerified () && !state.IsVerified ())
		return false;
	if (!_state.IsVerified () && state.IsVerified ())
		return true;
	if (_state.IsVoting () && !state.IsVoting ())
		return false;
	if (!_state.IsVoting () && state.IsVoting ())
		return true;
	if (_state.IsAdmin () && !state.IsAdmin ())
		return false;
	if (!_state.IsAdmin () && state.IsAdmin ())
		return true;
	return _id < info._id;
}

// Basic project member information

Tri::State Member::IsPrehistoricScript (GlobalId scriptId) const
{
	Assert (scriptId != gidInvalid);
	if (_preHistoricScript == gidInvalid)
		return Tri::Maybe;
	else if (scriptId <= _preHistoricScript)
		return Tri::Yes;
	else
		return Tri::No;
}

Tri::State Member::IsFutureScript (GlobalId scriptId) const
{
	Assert (scriptId != gidInvalid);
	if (_mostRecentScript == gidInvalid)
		return Tri::Maybe;
	else if (scriptId > _mostRecentScript)
		return Tri::Yes;
	else
		return Tri::No;
}

bool Member::IsIdentical (Member const & member) const
{
	return _id == member._id &&
		   _state.GetValue () == member._state.GetValue ();
}

void Member::SetMostRecentScript (GlobalId scriptId)
{
	// Never push the clock back
	Assert (scriptId != gidInvalid || _mostRecentScript == gidInvalid);
	if (_mostRecentScript == gidInvalid || scriptId == gidInvalid || _mostRecentScript < scriptId)
	{
		if (_mostRecentScript == gidInvalid &&
			_preHistoricScript != gidInvalid &&
			scriptId < _preHistoricScript)
			return;	// The first most recent script cannot be smaller then the pre-historic script id

		_mostRecentScript = scriptId;
	}
}

void Member::ResetScriptMarkers ()
{
	_preHistoricScript = gidInvalid;
	_mostRecentScript = gidInvalid;
}

void Member::Serialize (Serializer& out) const
{
	out.PutLong (_id);
	out.PutLong (_state.GetValue ());
	out.PutLong (_preHistoricScript);
	out.PutLong (_mostRecentScript);
}

void Member::Deserialize (Deserializer& in, int version)
{
	_id = in.GetLong ();
	_state = MemberState (in.GetLong ());
	if (version >= 38)
	{
		_preHistoricScript = in.GetLong ();
		_mostRecentScript = in.GetLong ();
	}
}

// Full project member information

MemberInfo::MemberInfo (std::string const & srcPath, File::Offset offset, int version)
{
	FileDeserializer in (srcPath);
	in.SetPosition (offset);
	Deserialize (in, version);
}

void MemberInfo::SetUserId (UserId id)
{
	_id = id;
	MemberId idStr (_id);
	_description.SetUserId (idStr.c_str ());
}

bool MemberInfo::IsIdentical (MemberInfo const & info) const
{
	return Member::IsIdentical (info) &&
		   _description.IsEqual (info._description);
}

void MemberInfo::Serialize (Serializer& out) const
{
	Member::Serialize (out);
    _description.Serialize (out);
}

void MemberInfo::Deserialize (Deserializer& in, int version)
{
	Member::Deserialize (in, version);
    _description.Deserialize (in, version);
}

void MemberInfo::Dump (XML::Node * parent) const
{
	XML::Node * member = parent->AddChild ("Member");
	member->AddAttribute ("id", ToHexString (Id ()));
	member->AddTransformAttribute ("name", Name ().c_str ());
	// Remove null padding from HubId by converting it to c_str and back
	member->AddTransformAttribute ("hubId", HubId ().c_str ());
	member->AddAttribute ("idString", ReplaceNullPadding<'.'> (Description ().GetUserId ()));
	XML::Node * state = member->AddEmptyChild ("State");
	state->AddAttribute ("status", State ().GetDisplayName ());
	if (State ().IsDead ())
	{
		if (State ().HasDefected ())
			state->AddAttribute ("defectScriptReceived", "yes");
		else
			state->AddAttribute ("defectScriptReceived", "no");
	}
	if (State ().IsVerified ())
		state->AddAttribute ("verified", "yes");
	else
		state->AddAttribute ("verified", "no");

	XML::Node * limitingScripts = member->AddEmptyChild ("LimitingScripts");
	limitingScripts->AddAttribute ("prehistoric", GlobalIdPack (GetPreHistoricScript ()).ToString ());
	limitingScripts->AddAttribute ("mostRecent", GlobalIdPack (GetMostRecentScript ()).ToString ());

	::License license (Description ().GetLicense ());
	if (license.IsValid ())
	{
		XML::Node * licenseNode = member->AddEmptyChild ("License");
		licenseNode->AddTransformAttribute ("licensee", license.GetLicensee ());
		licenseNode->AddAttribute ("seats"   , license.GetSeatCount ());
		licenseNode->AddAttribute ("version" , license.GetVersion ());
		licenseNode->AddAttribute ("product" , license.GetProductId ());
	}
}

std::ostream& operator<<(std::ostream& os, MemberInfo const & info)
{
	os << "| " << ToHexString (info.Id ());
	os << " | " << info.Name ();
	os << " | " << info.State ();
	os << " | " << ReplaceNullPadding<'.'> (info.HubId ()) << "; " << ReplaceNullPadding<'.'> (info.Description ().GetUserId ());
	os << " | " << GlobalIdPack (info.GetPreHistoricScript ());
	os << " | " << GlobalIdPack (info.GetMostRecentScript ());
	os << " | ";
	License license (info.Description ().GetLicense ());
	if (license.IsValid ())
		os << license.GetLicensee () << "; seats: " << license.GetSeatCount () << "; version: " << license.GetVersion ();
	else
		os << "UNLICENSED";
	os << " |";
	return os;
}
