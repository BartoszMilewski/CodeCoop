#if !defined (DATE_H)
#define DATE_H
// -----------------------------------
// (c) Reliable Software, 2003 -- 2004
// -----------------------------------

#include <Sys/SysTime.h>

class Date
{
public:
	Date ()	{ Clear ();	}
	Date (int month, int day, int year)
		: _month (month),
		  _day (day),
		  _year (year)
	{}
	int Year  () const { return _year;  }
	int Month () const { return _month; }
	int Day   () const { return _day;   }

	bool IsValid () const;

	bool operator == (Date const & d) const
	{
		return  (_year == d.Year ()) && 
			    (_month == d.Month ()) && 
				(_day == d.Day ()); 
	}

	void Now ();
	void Clear ()
	{
		_year  = 0;
		_month = 0;
		_day   = 0;
	}
	bool IsPast () const;
	void AddDays (int days);
	std::string ToString (bool shortFormat = false) const;

	static int DaysInMonth (int month, int year);
protected:
	int _year;
	int _month;
	int _day;
};

#endif
