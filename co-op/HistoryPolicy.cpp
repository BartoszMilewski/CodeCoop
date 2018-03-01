//---------------------------------------------
// (c) Reliable Software 2005
//---------------------------------------------

#include "precompiled.h"
#include "HistoryPolicy.h"
#include "HistoryNode.h"
#include "ProjectDb.h"

using namespace History;

bool MemberPolicy::XCanRequestResend (GlobalId unitId) const
{
	if (_projectDb.XIsProjectMember (unitId))
	{
		MemberState state = _projectDb.XGetMemberState (unitId);
		return !state.IsDead ();
	}

	return true;
}

bool MemberPolicy::XCanTerminateTree (Node const & node) const
{
	return node.IsDefect ();
}
