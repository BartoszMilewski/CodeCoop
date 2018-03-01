// (c) Reliable Software, 2004
#include <WinLibBase.h>
#include "PackedTime.h"

void PackedTimeStr::Init (FILETIME const * time, bool shortFormat)
{
	if (time->dwHighDateTime == 0 && time->dwLowDateTime == 0)
		return;

	// Convert UTC-based file time into a local file time. 
    FILETIME localFileTime;
	if (!::FileTimeToLocalFileTime (time, &localFileTime))
		memcpy (&localFileTime, &time, sizeof (FILETIME));
	// Convert local file time to the system time
	SYSTEMTIME sysTime;
	if (::FileTimeToSystemTime (&localFileTime, &sysTime))
	{
		// Format system time using locale information
		std::string date;
		std::string time;
		unsigned long flags = shortFormat ? DATE_SHORTDATE : DATE_LONGDATE;
		int dateLen = ::GetDateFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, 0, 0);
		if (dateLen > 0)
		{
			date.resize (dateLen);
			::GetDateFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, &date [0], date.size ());
			_timeStr = date;
			// APIs appended '\0' at the end of date
			_timeStr.replace (dateLen - 1, dateLen, 1, ',');
		}
		flags = 0;
		//flags = LOCALE_NOUSEROVERRIDE;
		if (shortFormat)
			flags |= TIME_NOSECONDS;
		int timeLen = ::GetTimeFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, 0, 0);
		if (timeLen > 0)
		{
			time.resize (timeLen);
			::GetTimeFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, &time [0], time.size ());
			_timeStr += ' ';
			_timeStr += time;
		}
	}
}
