#if !defined (SYSTIME_H)
#define SYSTIME_H
//----------------------------------
// (c) Reliable Software 2000 - 2005
//----------------------------------

namespace Win
{
	class Time
	{
	public:
		Time ()
		{
			::GetLocalTime (&_time);
		}
		int Year () const { return _time.wYear; }
		int Month () const { return _time.wMonth; }
		int Day () const { return _time.wDay; }
		int Hour () const { return _time.wHour; }
		int Minute () const { return _time.wMinute; }
		int Second () const { return _time.wSecond; }
	private:
		SYSTEMTIME _time;
	};

	class TimeZone
	{
	public:
		TimeZone ()
		{
			::ZeroMemory (&_zone, sizeof (TIME_ZONE_INFORMATION));
			::GetTimeZoneInformation (&_zone);
		}
		int GetBias () const {return _zone.Bias; }
	private:
		TIME_ZONE_INFORMATION	_zone;
	};
}

#endif
