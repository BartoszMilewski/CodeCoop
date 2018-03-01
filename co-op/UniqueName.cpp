//-----------------------------------------
// (c) Reliable Software 1998 -- 2001
//-----------------------------------------

#include "precompiled.h"
#include "UniqueName.h"

#include <File/Path.h>

#include <StringOp.h>

//
// UniqueName
//

void UniqueName::Serialize (Serializer& out) const
{
    out.PutLong (_gidParent);
    _name.Serialize (out);
}

void UniqueName::Deserialize (Deserializer& in, int version)
{
    _gidParent = in.GetLong ();
    _name.Deserialize (in, version);
}

bool UniqueName::IsEqual (UniqueName const & uname) const
{
    if (GetParentId () == uname.GetParentId ())
    {
		std::string const & thisName = GetName ();
		return IsFileNameEqual (thisName, uname.GetName ());
    }
	return false;
}

bool UniqueName::IsStrictlyEqual (UniqueName const & uname) const
{
    if (GetParentId () == uname.GetParentId ())
    {
		std::string const & thisName = GetName ();
		return IsCaseEqual (thisName, uname.GetName ());
    }
	return false;
}

bool UniqueName::IsNormalized () const
{
	return FilePath::IsFileNameOnly (_name);
}

bool UniqueName::operator < (UniqueName const & uname) const
{
	if (GetParentId () < uname.GetParentId ())
		return true;
	if (uname.GetParentId () < GetParentId ())
		return false;
	std::string const & thisName = GetName ();
	return IsFileNameLess (thisName, uname.GetName ());
}

