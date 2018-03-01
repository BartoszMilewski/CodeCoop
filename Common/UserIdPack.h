#if !defined (USERIDPACK_H)
#define USERIDPACK_H
//-------------------------------------
//  (c) Reliable Software, 1999 -- 2004
//-------------------------------------

#include "GlobalId.h"

#include <string>

class UserIdPack
{
public:
	UserIdPack (char const * userId)
	{
		Init (userId);
	}
	UserIdPack (std::string const & userId)
	{
		Init (userId);
	}

	std::string GetUserIdString () const { return _idStr; }
	UserId GetUserId () const { return _id; }
	bool IsRandom () const { return _idStr == "New"; } 

private:
	void Init (std::string const & str);

private:
	std::string	_idStr;
	UserId		_id;
};

#endif
