//---------------------------------------
//  (c) Reliable Software, 2004
//---------------------------------------

#include "precompiled.h"
#include "UserIdPack.h"

#include <StringOp.h>

void UserIdPack::Init (std::string const & str)
{
	if (str.empty () || str.length () < 4)
	{
		_idStr = str;
		_id = gidInvalid;
	}
	else if (str [0] == 'i' && str [1] == 'd' && str [2] == '-')
	{
		// Prefix 'id-' found
		if (str [3] == '*')
		{
			// id-*<random string>
			_idStr = "New";	
			_id = gidInvalid;
		}
		else if (str [3] == 'x')
		{
			// id-x<hex>
			std::istringstream in (str.substr (4));
			in >> std::hex >> _id;
			_idStr = str.substr (4);
		}
		else
		{
			// id-<decimal>
			std::string digits (str.substr (3));
			std::istringstream in (digits);
			if (digits.find_first_of ("abcdefABCDEF") != std::string::npos)
			{
				// During 4.5 Beta there was period when we used id strings with hex digits but without leading x
				in >> std::hex >> _id;
			}
			else
			{
				in >> std::dec >> _id;
			}
			_idStr = ToHexString (_id);
		}
	}
	else
	{
		_idStr = str;
		_id = gidInvalid;
	}
}
