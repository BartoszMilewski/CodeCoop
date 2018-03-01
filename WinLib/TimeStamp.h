#if !defined TIMESTAMP_H
#define TIMESTAMP_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <ctime>

unsigned long const Second = 1;
unsigned long const Minute = 60 * Second;
unsigned long const Hour = 60 * Minute;
unsigned long const Day = 24 * Hour;
unsigned long const Week = 7 * Day;

inline long CurrentTime ()
{
	time_t now;
	::time (&now);
	long longTime = static_cast<long> (now);
	Assert (longTime > 0);
	return longTime;
}

bool IsTimeOlderThan (long timeStamp, long period);

class StrTime
{
public:
    StrTime (long timeValue, char const * localeName = "")
    {
		Reset (timeValue, localeName);
	}
    long AsLong ()  const 
	{
		Assert (_value != 0);
		return static_cast<long>(_value);
	}
	void Reset (long value, char const * localeName = "");
	std::string const & GetString () const 
	{
		Assert (_value != 0);
		return _string;
	}
private:
	time_t		_value;
	std::string	_string;
};

class StrDate
{
public:
	StrDate (long timeValue, char const * localeName = "", bool inShortFormat = true)
	{
		Reset (timeValue, localeName, inShortFormat);
	}

	long AsLong ()  const 
	{
		Assert (_value != 0);
		return static_cast<long>(_value);
	}
	std::string const & GetString () const 
	{
		Assert (_value != 0);
		return _string;
	}

	void Reset (long value, char const * localeName = "", bool inShortFormat = true);

private:
	time_t		_value;
	std::string	_string;
};

class SmtpCurrentTime
{
public:
	SmtpCurrentTime ();
	std::string const & GetString () const 
	{
		return _string;
	}
private:
	std::string _string;
};

#endif
