//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#include "HistoryRange.h"

bool History::Range::IsEqual (Range const & range) const
{
	if (_fromCurrentVersion != range._fromCurrentVersion)
		return false;

	if (_ids.size () != range._ids.size ())
		return false;

	for (unsigned i = 0; i < _ids.size (); ++i)
	{
		if (_ids [i] != range._ids [i])
			return false;
	}

	return true;
}

bool History::Range::IsSubsetOf (GidList const & ids) const
{
	GidSet idsSet (ids.begin (), ids.end ());
	GidSet rangeSet (_ids.begin (), _ids.end ());
	GidSet result;
	std::set_difference (rangeSet.begin (),
						 rangeSet.end (),
						 idsSet.begin (),
						 idsSet.end (),
						 std::inserter (result, result.end ()));

	return result.empty ();
}
