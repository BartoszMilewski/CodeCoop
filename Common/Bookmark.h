#if !defined (BOOKMARK_H)
#define BOOKMARK_H
//------------------------------------
//  (c) Reliable Software, 1999 - 2006
//------------------------------------

#include "GlobalId.h"

//
// Bookmark used to mark rows in the table
//

class Bookmark
{
public:
	Bookmark (GlobalId gid, char const * name)
		: _gid (gid),
		  _name (name)
	{}
	Bookmark (GlobalId gid, std::string const & name)
		: _gid (gid),
		  _name (name)
	{}
	Bookmark (GlobalId gid)
		: _gid (gid)
	{}
	Bookmark (char const * name)
		: _gid (gidInvalid),
		  _name (name)
	{}
	Bookmark (std::string const & name)
		: _gid (gidInvalid),
		  _name (name)
	{}

	bool IsValidGid () const { return _gid != gidInvalid; }
	bool IsNameValid () const { return !_name.empty (); }
	GlobalId GetGlobalId () const { return _gid; }
	std::string const & GetName () const { return _name; }

private:
	GlobalId	_gid;
	std::string	_name;
};

#endif
