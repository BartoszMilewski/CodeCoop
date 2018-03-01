#if !defined (MEMBERID_H)
#define MEMBERID_H
//-------------------------------------
//  (c) Reliable Software, 1999 -- 2004
//-------------------------------------

#include "GlobalId.h"

#include <string>
#include <StringOp.h>

class MemberId
{
public:
	MemberId ()
		: _maxCnt (16)
	{
		_userId.assign ("id-*");
		for (int charCnt = 0; charCnt < _maxCnt; ++charCnt)
		{
			unsigned int random = rand ();
			unsigned int charCode = random % (0x7f - 0x20) + 0x20;
			_userId += ToString (char (charCode));
		}
	}
	MemberId (UserId userId)
		: _maxCnt (16)
	{
		Assert (IsValidUid (userId));
		_userId.assign ("id-x");
		_userId += ToHexString (userId);
	}

	char const * c_str () const { return _userId.c_str (); }
	std::string const & Get () const { return _userId; }

private:
	std::string	_userId;
	int const	_maxCnt;
};


#endif
