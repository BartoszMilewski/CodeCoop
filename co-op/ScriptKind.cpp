//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "precompiled.h"
#include "ScriptKind.h"

void ScriptKind::InitFromOldScriptKind (unsigned long oldKind)
{
	if (oldKind == 1)
	{
		// Change script
		_value = ScriptKindSetChange ().GetValue ();
	}
	else if (oldKind == 2)
	{
		// Control script -- this all we can do!
		_bits._level0 = 1;
	}
	else if (oldKind == 3)
	{
		// Full synch script
		_value = ScriptKindFullSynch ().GetValue ();
	}
	else
	{
		Assert (!"ScriptKind::InitFromOldScriptKind -- illegal old script kind");
	}
}

bool ScriptKind::Verify (bool isFromVersion40) const
{
	if (_value > 0x1f)
		return false;

	if (isFromVersion40)
	{
		if (IsAck () ||
			IsSetChange () ||
			IsAddMember () ||
			IsEditMember () ||
			IsDeleteMember () ||
			IsJoinRequest () ||
			IsFullSynch ())
			return true;
	}
	else if (IsAck () ||
			 IsSetChange () ||
			 IsScriptResendRequest () ||
			 IsFullSynchResendRequest () ||
			 IsJoinRequest () ||
			 IsAddMember () ||
			 IsEditMember () ||
			 IsDeleteMember () ||
			 IsUnitChange () ||
			 IsFullSynch () ||
			 IsVerificationPackage ())
			return true;

	return false;
}

std::ostream & operator<<(std::ostream & os, ScriptKind kind)
{
	if (kind.IsAck ())
		os << "acknowledgement";
	else if (kind.IsScriptResendRequest ())
		os << "script re-send request";
	else if (kind.IsFullSynchResendRequest ())
		os << "full synch re-send request";
	else if (kind.IsJoinRequest ())
		os << "join request";
	else if (kind.IsAddMember ())
		os << "add member";
	else if (kind.IsEditMember ())
		os << "edit member";
	else if (kind.IsDeleteMember ())
		os << "delete member";
	else if (kind.IsSetChange ())
		os << "set change";
	else if (kind.IsUnitChange ())
		os << "unit change";
	else if (kind.IsFullSynch ())
		os << "full sync package";
	else if (kind.IsVerificationPackage ())
		os << "project verification package";
	else
		os << "unknown script kind";
	return os;
}