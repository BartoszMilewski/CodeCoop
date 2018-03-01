//------------------------------------
//  (c) Reliable Software, 2006 - 2008
//------------------------------------

#include <WinLibBase.h>
#include "TimeStamp.h"
#include "Sys/SysTime.h"

#include <locale>
#include <sstream>

bool IsTimeOlderThan (long timeStamp, long period)
{
	time_t now;
	::time (&now);
	time_t then = static_cast<time_t> (timeStamp);
	double elapsedTime = ::difftime (now, then);
	return elapsedTime > period;
}

void StrTime::Reset (long value, char const * localeName)
{
	_value = static_cast<time_t> (value);
	if (value == 0)
	{
		_string.clear ();
		return;
	}

	tm * tmValue = ::localtime (&_value);
	if (tmValue == 0)
	{
		_string.assign ("???");
		return;
	}

	std::locale loc (localeName);
	std::time_put<char> const & tp = std::use_facet<std::time_put<char> >(loc);
	std::ostringstream out;
	out.imbue (loc);
	char format [] = "%a, %c";
	tp.put (std::ostream::_Iter (out.rdbuf ()), out, ' ', 
			tmValue, format, format + sizeof (format) - 1);
	_string = out.str ();
}

void StrDate::Reset (long value, char const * localeName, bool inShortFormat)
{
	_value = static_cast<time_t> (value);
	if (value == 0)
	{
		_string.clear ();
		return;
	}

	tm * tmValue = ::localtime (&_value);
	if (tmValue == 0)
	{
		_string.assign ("???");
		return;
	}

	std::locale loc (localeName);
	std::time_put<char> const & tp = std::use_facet<std::time_put<char> >(loc);
	std::ostringstream out;
	out.imbue (loc);
	char shortFormat [] = "%x";
	char longFormat [] = "%#x";
	if (inShortFormat)
		tp.put (std::ostream::_Iter (out.rdbuf ()), out, ' ', 
				tmValue, shortFormat, shortFormat + sizeof (shortFormat) - 1);
	else
		tp.put (std::ostream::_Iter (out.rdbuf ()), out, ' ', 
				tmValue, longFormat, longFormat + sizeof (longFormat) - 1);

	_string = out.str ();
}

SmtpCurrentTime::SmtpCurrentTime ()
{
	time_t now;
	::time (&now);
	tm * tmValue = ::localtime (&now);

	std::locale loc ("C");
	std::time_put<char> const & tp = std::use_facet<std::time_put<char> >(loc);
	std::ostringstream out;
	out.imbue (loc);
	// "Tue, 24 Oct 2006 09:11:33 +0480"
	char format [] = "%a, %d %b %Y %X";
	tp.put (std::ostream::_Iter (out.rdbuf ()), out, ' ', 
			tmValue, format, format + sizeof (format) - 1);
	out << ' ';
	Win::TimeZone zone;
	long bias = zone.GetBias ();
	out.width (5); // four digits and sign
	out.fill ('0'); // fill with zeros
	out.setf (std::ios_base::internal); // left adjust sign, right adjust number
	out.setf (std::ios_base::showpos); // show plus sign
	out << bias;

	_string = out.str ();
}
