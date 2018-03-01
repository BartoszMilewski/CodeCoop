// -----------------------------
// (c) Reliable Software, 1999-2003
// -----------------------------
#include "precompiled.h"
#include "SelectIter.h"
#include "RecordSet.h"
#include "SelectionMan.h"
#include <Dbg/Assert.h>

SelectionSeq::SelectionSeq (SelectionMan const * selMan, char const * tableName)
: _recordSet (selMan->GetRecordSet (tableName)),
  _restriction (selMan->GetRestriction ()),
  _cur (0)
{
    selMan->GetSelectedRows (_rows);
}

void SelectionSeq::Advance ()
{
	Assert (!AtEnd ());
	_cur ++;
}

int SelectionSeq::GetId () const
{ 
	Assert (!AtEnd ());
	return _recordSet->GetId (_rows [_cur]); 
}

std::string const & SelectionSeq::GetName () const
{
	Assert (!AtEnd ());
	return _recordSet->GetName (_rows [_cur]);
}

TripleKey const & SelectionSeq::GetKey () const
{
	Assert (!AtEnd ());
	return _recordSet->GetKey (_rows [_cur]);
}
