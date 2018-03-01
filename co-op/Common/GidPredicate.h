#if !defined (GIDPREDICATE_H)
#define GIDPREDICATE_H
//
// (c) Reliable Software 1998 -- 2002
//

#include "GlobalId.h"

class GidPredicate
{
public:
	virtual ~GidPredicate () {}

	virtual bool operator () (GlobalId gid) const { return false; }
};

class IsThisUser : public GidPredicate
{
public:
	IsThisUser (UserId userId)
		: _userId (userId)
	{
		Assert (IsValidUid (userId));
	}

	bool operator () (GlobalId gid) const
	{
		GlobalIdPack pack (gid);
		return pack.GetUserId () == _userId;
	}

private:
	UserId _userId;
};

class IsThisProject : public GidPredicate
{
public:
	IsThisProject (int projectId)
		: _projectId (projectId)
	{}

	bool operator () (GlobalId gid) const
	{
		return gid == _projectId;
	}

private:
	int _projectId;
};

#endif
