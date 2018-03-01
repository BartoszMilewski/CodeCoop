//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "MergeStatus.h"

char const * MergeStatus::GetStatusName () const
{
	if (IsIdentical ())
		return "Identical";
	else if (IsDifferent ())
		return "Different";
	else if (IsCreated ())
		return "New at source";
	else if (IsDeletedAtTarget ())
		return "Deleted at target";
	else if (IsDeletedAtSource ())
		return "Deleted at source";
	else if (IsAbsent ())
		return "Absent";
	else if (IsMergeParent ())
		return "Merge parent first";
	else if (IsMerged ())
		return "Merged";
	else if (IsConflict ())
		return "Conflict";

	return "";
}
