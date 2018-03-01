//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"
#include "WikiLexer.h"
#include <StringOp.h>

void WikiLexer::Accept (unsigned count)
{
	Assert (count > 0);
	_cur += count - 1;
	Accept ();
}

void WikiLexer::EatWhite ()
{
	while (Get () == ' ' || Get () == '\t')
		Accept ();
	if (Get () == '\n')
		Accept ();
}

void WikiLexer::EatWhiteToEnd ()
{
	while (Get () == ' ' || Get () == '\t')
		Accept ();
}

void WikiLexer::Accept ()
{
	++_cur;
	if (_cur == _line.size ())
	{
		_cur = 0;
		// doesn't include '\n'
		if (!std::getline (_in, _line))
			_atEnd = true;
		_line += '\n';
	}
}

unsigned WikiLexer::Count (char c)
{
	Assert (_line.size () > 0);
	Assert (_line [_cur] == c);
	unsigned cur = _cur;
	unsigned end = _line.size () - 1; // without \n
	do
	{
		++cur;
	} while (cur < end && _line [cur] == c);
	return cur - _cur;
}

bool WikiLexer::IsMatch (std::string const & pattern, bool strict)
{
	Assert (_line.size () > 0);
	unsigned count = _line.size () - _cur - 1; // without \n
	if (count < pattern.size ())
		return false;
	if (strict)
		return pattern == _line.substr (_cur, pattern.size ());
	else
		return IsNocaseEqual (_line.substr (_cur, pattern.size ()), pattern);
}

inline bool IsOneOf (char c, char const list [])
{
	while (*list != '\0' && *list != c)
		++list;
	return *list == c;
}

std::string WikiLexer::GetString (char const stopChars [])
{
	std::string str;
	EatWhite ();
	char c = Get ();
	while (c != 0 && c != '\n' && !IsOneOf (c, stopChars))
	{
		if (c == ' ' || c == '\t')
			break;
		str += c;
		Accept ();
		c = Get ();
	}
	return str;
}

std::string WikiLexer::GetQuotedString ()
{
	Assert (Get () == '"');
	Accept ();
	std::string str;
	char c = Get ();
	while (c != 0 && c != '\n' && c != '"')
	{
		str += c;
		Accept ();
		c = Get ();
	}
	if (c == '"')
		Accept ();
	return str;
}
