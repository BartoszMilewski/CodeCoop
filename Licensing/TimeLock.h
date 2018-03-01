#if !defined (TIMELIMIT_H)
#define TIMELIMIT_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2008
//------------------------------------

#include <Sys/Date.h>

class TimeLimit
{
public:
	TimeLimit ()
		: _warning (07, 15, 2008), _expiration (07, 21, 2008)
	{}
	bool AboutToExpire () const
	{
		return _warning.IsPast ();
	}
    bool HasExpired () const
    {
		return _expiration.IsPast ();
    }
	std::string GetExpirationDateStr () const { return _expiration.ToString (); }

private:
	Date _warning;
	Date _expiration;
};

#endif
