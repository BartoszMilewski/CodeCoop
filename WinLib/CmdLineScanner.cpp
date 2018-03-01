//----------------------------------
//  (c) Reliable Software, 1999-2003
//----------------------------------
#include <WinLibBase.h>
#include "CmdLineScanner.h"

//
// Command Line Scanner
// Detects 
// Switch tokens (either - or /)
// Colons :
// Quoted or non-quoted strings
//

char const * const CmdLineScanner::whiteChars = " \t\r\n";
char const * const CmdLineScanner::stopChars = "-/: \t\r\n";

std::string CmdLineScanner::GetString ()
{
	Assert (_token == tokString);
	return _str.substr (_begString, _endString - _begString);
}

void CmdLineScanner::Accept ()
{
	if (!EatWhite ())
	{
		_token = tokEnd;
		return;
	}

	switch (_str [_cur])
	{
	case '-':
	case '/':
		_token = tokSwitch;
		_cur++;
		break;
	case ':':
		_token = tokColon;
		_cur++;
		break;
	case '"':
		_token = tokString;
		_cur++;
		_begString = _cur;
		_endString = _str.find ('"', _cur);
		if (_endString == std::string::npos)
		{
			_endString = _str.length ();
			_cur = _endString;
		}
		else
			_cur = _endString + 1; // eat closing quote
		break;
	default:
		_token = tokString;
		_begString = _cur;
		_endString = _str.find_first_of (stopChars, _cur);
		if (_endString == std::string::npos)
			_endString = _str.length ();
		_cur = _endString;
	}

}

bool CmdLineScanner::EatWhite ()
{
	_cur = _str.find_first_not_of (whiteChars, _cur);
	return _cur != std::string::npos;
}
