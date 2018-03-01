// -----------------------------
// (c) Reliable Software 2004-06
// -----------------------------

#include <WinLibBase.h>
#include "Date.h"

int MonthDays [] =
{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

inline bool IsLeapYear (int year)
{
	return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int Date::DaysInMonth (int month, int year)
{
	int days = MonthDays [month];
	if (month == 2 && IsLeapYear (year))
		++days;
	return days;
}

void Date::Now ()
{
	Win::Time today;
	_year  = today.Year ();
	_month = today.Month ();
	_day   = today.Day ();
}

bool Date::IsPast () const
{
	Assert (IsValid ());
	Win::Time today;

	if (_year != today.Year ())
		return _year < today.Year ();

	if (_month != today.Month ())
		return _month < today.Month ();

	return _day < today.Day ();
}

bool Date::IsValid () const
{
	return _year >= 1 
		&& _month >= 1 
		&& _month <= 12 
		&& _day >= 1 
		&& _day <= DaysInMonth (_month, _year);
}

void Date::AddDays (int days)
{
	Assert (days >= 0);
	Assert (IsValid ());
	_day += days;
	while (_day > DaysInMonth (_month, _year))
	{
		_day -= DaysInMonth (_month, _year);
		++_month;
		if (_month > 12)
		{
			++_year;
			_month = 1;
		}
	}
	Assert (IsValid ());
}

std::string Date::ToString (bool shortFormat) const
{
	Assert (IsValid ());
	SYSTEMTIME sysTime;
	memset (&sysTime, 0, sizeof (SYSTEMTIME));
	sysTime.wDay = _day;
	sysTime.wMonth = _month;
	sysTime.wYear = _year;
	// Format date using locale information
	std::string date;
	unsigned long flags = shortFormat ? DATE_SHORTDATE : DATE_LONGDATE;
	int dateLen = ::GetDateFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, 0, 0);
	if (dateLen > 0)
	{
		date.resize (dateLen);
		::GetDateFormat (LOCALE_USER_DEFAULT, flags, &sysTime, 0, &date [0], date.size ());
		// APIs appended '\0' at the end of date
		date.replace (dateLen - 1, dateLen, 1, ' ');
	}
	return date;
}

namespace UnitTest
{
	void DateTest (std::ostream & out)
	{
		out << std::endl << "Test of Date class." << std::endl;
		
		{
			// sanity checks
			Date test;
			Assert (!test.IsValid ());
			
			test.Now ();
			Assert (test.IsValid ());
			Assert (!test.IsPast ());
			Assert (!test.ToString ().empty ());
			
			test.Clear ();
			Assert (!test.IsValid ());
		}
		// explicit tests of specific dates
		{
			Assert (IsLeapYear (1600));
			Assert (!IsLeapYear (1700));
			Assert (!IsLeapYear (1800));
			Assert (!IsLeapYear (1900));
			Assert (IsLeapYear (2000));
			Assert (IsLeapYear (2004));
			Assert (!IsLeapYear (2100));

			Date test2001 (2, 28, 2001);
			Assert (!IsLeapYear (test2001.Year ()));
			test2001.AddDays (1);
			Assert (test2001.Day () == 1);
			Assert (test2001.Month () == 3);

			Date test2004 (2, 28, 2004);
			test2004.AddDays (1);
			Assert (test2004.Day () == 29);
			Assert (test2004.Month () == 2);
			test2004.AddDays (1);
			Assert (test2004.Day () == 1);
			Assert (test2004.Month () == 3);
		}
		// arithmetics	
		{
			// adding 0 days and 1 day
			Date test (1, 1, 1);
			for (int i = 1; i < 40000; ++i) // more than a century
			{
				Assert (test.IsValid ());
				Date testOrig (test.Month (), test.Day (), test.Year ());
				test.AddDays (0);
				Assert (test == testOrig);

				test.AddDays (1);
				Assert (test.Day () != testOrig.Day ());
				if (testOrig.Day () < Date::DaysInMonth (testOrig.Month (), testOrig.Year ()))
				{
					Assert (test.Day () == (testOrig.Day () + 1));
				}
				else
				{
					Assert (test.Day () == 1);
					if (testOrig.Month () < 12)
					{
						Assert (test.Month () == (testOrig.Month () + 1));
						Assert (test.Year () == testOrig.Year ());
					}
					else
					{
						Assert (test.Month () == 1);
						Assert (test.Year () == (testOrig.Year () + 1));
					}
				}
			}
		}
		{
			// adding multiple days: compare results of two different methods
			Date test1 (1, 1, 1);
			for (int i = 1; i < 40000; ++i) // more than a century
			{
				test1.AddDays (1);

				Date test2 (1, 1, 1);
				test2.AddDays (i);
				Assert (test2.IsValid ());
				Assert (test1 == test2);
			}
		}
		out << "Passed." << std::endl;
	}
}
