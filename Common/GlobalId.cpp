//-------------------------------------------
//  GlobalId.cpp
//  (c) Reliable Software, 1998 -- 2004
//-------------------------------------------

#include "precompiled.h"
#include "GlobalId.h"
#include <sstream>

GlobalId RandomId ()
{
	return scriptIdMask | (rand () & ~scriptIdMask);
}

GlobalId RandomId (UserId userId)
{
	Assert (IsValidUid (userId));
	GlobalIdPack gid (userId, (rand () & ~scriptIdMask));
	return gid;
}

GlobalIdPack::GlobalIdPack (std::string const & gidStr)
{
	size_t endPos = gidStr.find_first_not_of ("0123456789abcdefABCDEF-");
	if (endPos != std::string::npos && endPos != 8)
		throw Win::InternalException ("Bad global ID format", gidStr.c_str ());
	std::istringstream in (gidStr);
	if (gidStr.find ('-') == std::string::npos)
	{
		// Plain hex number
		in >> std::hex >> _gid;
	}
	else
	{
		// Formated gid "xx-yyy"
		unsigned int userId;
		unsigned int ordinal;
		char dash;
		in >> std::hex >> userId;
		in >> dash;
		Assert (dash == '-');
		in >> std::hex >> ordinal;
		_gid = (userId << 20) | ordinal;
	}
}

std::string GlobalIdPack::ToString () const
{
    std::ostringstream buffer;
	buffer << std::hex << GetUserId () << "-" << std::hex << GetOrdinal ();
    return buffer.str ();
}

std::string GlobalIdPack::ToBracketedString () const
{
    std::ostringstream buffer;
	buffer << '<' << std::hex << GetUserId () << "-" << std::hex << GetOrdinal () << '>';
    return buffer.str ();
}

std::string GlobalIdPack::ToSquaredString () const
{
    std::ostringstream buffer;
	buffer << '[' << std::hex << GetUserId () << "-" << std::hex << GetOrdinal () << ']';
    return buffer.str ();
}

std::ostream & operator<<(std::ostream & os, GlobalIdPack id)
{
	os << id.ToString ();
	return os;
}
