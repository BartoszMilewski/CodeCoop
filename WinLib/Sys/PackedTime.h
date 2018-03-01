#if !defined (PACKEDTIME_H)
#define PACKEDTIME_H
// (c) Reliable Software, 2004
#include <LargeInt.h>

// Packed time interval, given in days and minutes, can be added to PackedTime

class PackedTimeInterval
{
	friend class PackedTime;
public:
	PackedTimeInterval (int days, int min = 0)
	{
		Assert (days < 10000);
		int sec = days * 24 * 60 * 60 + min * 60;
		LargeInteger large (10000000, 0);
		large *= sec;
		_time.dwHighDateTime = large.High ();
		_time.dwLowDateTime = large.Low ();
	}
	operator FILETIME const * () const { return &_time; }
private:
	FILETIME _time;
};

// Packed time uses only two longs (packed into FILETIME) for storing date/time
// Good for arithmetic (with PackedTimeInterval) and comparison

class PackedTime
{
public:
	PackedTime ()
	{
		Clear ();
	}
	PackedTime (FILETIME time)
		:_time (time)
	{}
	void Now ()
	{
		::GetSystemTimeAsFileTime (&_time);
	}
	void Clear ()
	{
		memset (&_time, 0, sizeof (FILETIME));
	}
	bool IsEqual (PackedTime const & time) const
	{
		return ::CompareFileTime (&_time, &time._time) == 0;
	}
	int Compare (PackedTime const & time) const
	{
		return ::CompareFileTime (&_time, &time._time);
	}
	bool operator < (PackedTime const & time) const
	{
		return ::CompareFileTime (&_time, &time._time) < 0;
	}
	PackedTime operator += (PackedTimeInterval interval)
	{
		LargeInteger large (_time.dwLowDateTime, _time.dwHighDateTime);
		LargeInteger addend (interval._time.dwLowDateTime, interval._time.dwHighDateTime);
		large += addend;
		_time.dwLowDateTime = large.Low ();
		_time.dwHighDateTime = large.High ();
		return *this;
	}
	operator FILETIME const * () const { return &_time; }
protected:
	FILETIME	_time;
};

// CurrentPackedTime is initialized to Now

class CurrentPackedTime : public PackedTime
{
public:
	CurrentPackedTime ()
	{
		::GetSystemTimeAsFileTime (&_time);
	}
};

// Note: if PackedTime is not set (default constructor was used), the string will be empty
class PackedTimeStr
{
public:
	PackedTimeStr (PackedTime const & time, bool shortFormat = false)
	{
		Init (time, shortFormat);
	}
	char const * c_str () const { return _timeStr.c_str (); }
	unsigned int length () const { return _timeStr.length (); }
	std::string ToString () const { return _timeStr; }
private:
	void Init (FILETIME const * time, bool shortFormat);
	std::string	_timeStr;
};

#endif
