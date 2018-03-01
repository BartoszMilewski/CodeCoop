#if !defined (GLOBALID_H)
#define GLOBALID_H
//
// (c) Reliable Software 1997 -- 2004
//
#include <iosfwd>

// File id of global scope
// Contains user id.

typedef unsigned int GlobalId;
const GlobalId gidInvalid = 0xffffffff;
const GlobalId gidMarker = 0xfffffffe;	// First interesting script id marker used in the history trees
const GlobalId gidRoot = 0;				// Project root folder global id -- the very first
										// global id used when new project is created
const unsigned scriptIdMask = 0xfff00000;
const unsigned OrdinalInvalid = 0x000fffff;

typedef unsigned int UserId;
inline bool IsValidUid (UserId id) { return id == gidInvalid || id <= 0xfff; }
// helper functions for global id generation
GlobalId RandomId ();
GlobalId RandomId (UserId userId);

// Temporary class used for
// creating, unpacking and displaying GlobalId

class GlobalIdPack
{
public:
	GlobalIdPack (GlobalId gid)
		: _gid (gid)
	{}
	GlobalIdPack (UserId idUser, unsigned ordinal)
		: _gid ((idUser << 20) | ordinal)
	{}
	
	// Can accept GID in a file name--ignores extension
	GlobalIdPack (std::string const & str);

	bool IsFromJoiningUser () const { return _gid == scriptIdMask; }
	operator GlobalId () const
	{
		return _gid;
	}
	UserId GetUserId () const 
	{
		return (_gid >> 20) & 0xfff;
	}
	unsigned int GetOrdinal () const
	{
		return _gid & 0x000fffff;
	}
	std::string ToString () const;
	std::string ToBracketedString () const;
	static char const GetLeftBracket () { return '<'; }
	static char const GetRightBracket () { return '>'; }
	std::string ToSquaredString () const;

private:
	GlobalId	_gid;
};

// Popular types

typedef std::vector<int> IdList;
typedef std::vector<GlobalId> GidList;
typedef std::set<GlobalId> GidSet;
typedef std::vector<UserId> UserIdList;
typedef std::set<UserId> UserIdSet;

std::ostream & operator<<(std::ostream & os, GlobalIdPack id);
#endif
