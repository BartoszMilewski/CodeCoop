// ---------------------------
// (c) Reliable Software, 2001-2003
// ---------------------------
#include "precompiled.h"
#include "RecordSet.h"
#include <Dbg/Assert.h>

std::string RecordSet::_emptyStr;
TripleKey	RecordSet::_emptyKey;

int RecordSet::GetId (unsigned int row) const
{
	Assert (!"GetId not supported");
	return -1;
}

std::string const & RecordSet::GetName (unsigned int row) const
{
	Assert (!"GetName not supported");
	return _emptyStr;
}

TripleKey const & RecordSet::GetKey (unsigned int row) const
{
	Assert (!"GetKey not supported");
	return _emptyKey;
}
