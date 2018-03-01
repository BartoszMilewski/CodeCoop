#if !defined (ALERTLOG_H)
#define ALERTLOG_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "Table.h"
#include <File/Path.h>

class AlertLog : public Table
{
	class AlertData
	{
	public:
		AlertData ()
			: _count (0),
			  _latestLine (0)
		{}
		AlertData (unsigned int count, std::string const & date, unsigned int line)
			: _count (count),
			  _date (date),
			  _latestLine (line)
		{}
		void Update (std::string const & date, unsigned int latestLine)
		{
			_count++;
			_date = date;
			_latestLine = latestLine;
		}
	public:
		unsigned int _count;
		std::string  _date;
		unsigned int _latestLine;
	};

	// alert first line --> (count, last occurrence, line number)
	typedef std::map<std::string, AlertData> AlertMap;

public:
	void SetPath (char const * path) 
	{
		Assert (path != 0);
		FilePath logFolder (path);
		_logPath = logFolder.GetFilePath (_logName);
	}

	void Add (std::string const & alert);
	void Clear ();
	void View ();
	bool IsEmpty () const { return _alerts.empty (); }

	// Table interface
	void QueryUniqueIds (std::vector<int> & uids, Restriction const * restrict = 0);
	std::string	GetStringField (Column col, int uid) const;
	int GetNumericField (Column col, int uid) const;
private:
	void ReadLogFile (AlertMap & alertMap);
private:
	class AlertEntry
	{
	public:
		AlertEntry (std::string const & alert, 
					unsigned int count, 
					std::string const & date, 
					unsigned int line)
			: _alert (alert),
			  _count (count),
			  _date (date),
			  _latestLine (line)
		{}

		std::string	 _alert;
		unsigned int _count;
		std::string  _date;
		unsigned int _latestLine;
	};

	static bool IsLastLater (AlertEntry const & a1, AlertEntry const & a2)
	{
		return a1._latestLine > a2._latestLine;
	}
private:
	std::string	_logPath;

	std::vector<AlertEntry>	_alerts;

	static const char _logName [];
};

#endif
