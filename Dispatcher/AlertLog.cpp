// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "precompiled.h"
#include "AlertLog.h"
#include "OutputSink.h"
#include <File/FileIo.h>
#include <Com/Shell.h>
#include <File/Path.h>
#include <Ex/Error.h>
#include <TimeStamp.h>

// Alert log has format of a wiki table

// Log   := [Alert]
// Alert := "| " + Date + " | " + Text
// Text  := ["\n " + line of text + ' '] + '|'

// Example:
// | 06.06.06 | 
//  text of alert 1 (one-liner)|
// | 07.06.06 | 
//  line1 of alert 2 
//  line2 of alert 2 
//  line3 of alert 2|
// | 08.06.06 | 
//  text of alert 3|


const char AlertLog::_logName [] = "AlertLog.txt";

void AlertLog::Add (std::string const & alert)
{
	Assert (!_logPath.empty ());
	OutStream log (_logPath, std::ios_base::out | std::ios_base::app);
	StrTime now (CurrentTime ());
	log << "| " << now.GetString () << " | \n ";

	std::string::size_type curr = 0;
	std::string::size_type end = 0;
	do
	{
		end = alert.find ('\n', curr);
		if (end == std::string::npos)
		{
			log << alert.c_str () + curr;
			break;
		}
		log << alert.substr (curr, end - curr) << "\n ";
		curr = end + 1;
	}while (true);
	
	log << '|' << std::endl;
}

void AlertLog::Clear ()
{
	Assert (!_logPath.empty ());
	File::DeleteNoEx (_logPath);
}

void AlertLog::View ()
{
	Assert (!_logPath.empty ());
	int errCode = ShellMan::Open (0, _logPath.c_str ());
	if (errCode != ShellMan::Success)
	{
		SysMsg errInfo (errCode);
		std::string info = "Problem opening the log file:\n";
		info += _logPath;
		info += ".\n\nSystem tells us:\n";
		info += errInfo.Text ();
		TheOutput.Display (info.c_str (), Out::Error);
	}
}

// Table interface

void AlertLog::QueryUniqueIds (std::vector<int> & uids, Restriction const * restrict)
{
	Assert (!_logPath.empty ());

	uids.clear ();
	_alerts.clear ();

	AlertMap alertMap;
	ReadLogFile (alertMap);
	unsigned int const alertMapSize = alertMap.size ();
	if (alertMapSize == 0)
		return;

	_alerts.reserve (alertMapSize);
	for (AlertMap::const_iterator it = alertMap.begin ();
		 it != alertMap.end ();
		 ++it)
	{
		AlertData const & alert = it->second;
		_alerts.push_back (AlertEntry (it->first, alert._count, alert._date, alert._latestLine));
	}
	std::sort (_alerts.begin (), _alerts.end (), IsLastLater);
	uids.resize (alertMapSize);
	for (unsigned int i = 0; i < alertMapSize; ++i)
		uids [i] = i;
}

std::string	AlertLog::GetStringField (Column col, int uid) const
{
	Assert (col == Table::colDate || Table::colComment);
	if (col == Table::colDate)
		return _alerts [uid]._date;
	else
		return _alerts [uid]._alert;
}

int	AlertLog::GetNumericField (Column col, int uid) const
{
	Assert (col == Table::colCount);
	return _alerts [uid]._count;
}

void AlertLog::ReadLogFile (AlertMap & alertMap)
{
	InStream log (_logPath);
	std::string line;
	unsigned int lineCount = 0;
	while (std::getline (log, line))
	{
		// "| "
		if (line.size () < 2 || line [0] != '|' || line [1] != ' ')
			continue;

		// Date + " |"
		std::string::size_type dateEndPos = line.find ('|', 2);
		if (dateEndPos == std::string::npos)
			continue;

		std::string date = line.substr (2, dateEndPos - 3);

		// ' '
		if (dateEndPos + 2 != line.size () || line [dateEndPos + 1] != ' ')
			continue;

		// ' ' + text + [|]
		if (!std::getline (log, line))
			continue;

		if (line.empty () || line [0] != ' ')
			continue;

		line = line.substr (1); // eat ' '

		if (line [line.length () - 1] == '|')
			line.resize (line.size () - 1);

		AlertMap::iterator it = alertMap.find (line);
		if (it == alertMap.end ())
		{
			alertMap [line] = AlertData (1, date, lineCount);
		}
		else
		{
			AlertData & alertData = it->second;
			alertData.Update (date, lineCount);
		}
		lineCount ++;
	}
}