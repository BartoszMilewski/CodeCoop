#if !defined (ADMINELECTION_H)
#define ADMINELECTION_H
//-------------------------------------
//  (c) Reliable Software, 1999 -- 2002
//-------------------------------------

#include "GlobalId.h"
#include "MemberInfo.h"

#include <auto_vector.h>

class Project::Db;

//
// Project Admin election
//

class AdminElection
{
public:
	AdminElection (Project::Db const & projectDb);

	bool ProjectHasAdmin () const { return _curAdmin != 0; }
	bool IsNewAdminElected () const { return _newAdmin != 0; }
	unsigned int GetMemberCount () const { return _memberData.size (); }
	MemberInfo const & GetCurAdmin () const { return *_curAdmin; }
	MemberInfo const & GetNewAdmin () const { return *_newAdmin; }

private:
    MemberInfo const *		_curAdmin;
	MemberInfo const *		_newAdmin;
    std::vector<MemberInfo>	_memberData;
};

#endif
