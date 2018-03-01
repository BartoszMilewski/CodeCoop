// (c) Reliable Software 2003
#include <WinLibBase.h>
#include "WildCard.h"
#include <StringOp.h>

FileMultiMatcher::FileMultiMatcher (char const * multiPattern)
{
	char const * begin = multiPattern;
	char const * endMulti = multiPattern + strlen (multiPattern);
	char const * end;
	while (begin != endMulti)
	{
		begin = std::find_if (begin, endMulti, std::not1 (std::ptr_fun (::IsSpace)));
		if (begin == endMulti)
			break;
		if (*begin == '"')
		{
			++begin;
			end = std::find (begin, endMulti, '"');
			if (end == endMulti)
			{
				_isOk = false;
				return;
			}
			FileMatcher matcher (begin, end - begin);
			if (!matcher.IsOk ())
			{
				_isOk = false;
				return;
			}
			_matchers.push_back (matcher);
			++end;
		}
		else
		{
			end = std::find_if (begin, endMulti, ::IsSpace);
			FileMatcher matcher (begin, end - begin);
			if (!matcher.IsOk ())
			{
				_isOk = false;
				return;
			}
			_matchers.push_back (matcher);
		}
		begin = end;
	}
}

bool FileMultiMatcher::IsMatch (char const * name) const
{
//	return _matchers.end () != std::find_if (_matchers.begin (), _matchers.end (), 
//		std::bind2nd (std::mem_fun_ref (&FileMatcher::IsMatch), name)));

	std::vector<FileMatcher>::const_iterator it;
	for (it = _matchers.begin (); it != _matchers.end (); ++it)
	{
		if (it->IsMatch (name))
			return true;
	}
	return false;
}

FileMatcher::FileMatcher (char const * pattern, int len)
: _star1 (false), _dot (false), _star2 (false)
{
	_isOk = Parse (pattern, pattern + len);
}

FileMatcher::FileMatcher (char const * pattern)
: _star1 (false), _dot (false), _star2 (false)
{
	_isOk = Parse (pattern, pattern + strlen (pattern));
}

bool FileMatcher::Parse (char const * begin, char const * end)
{
	// Prefix
	char const * p = begin;
	while (p != end)
	{
		if (*p == '*' || *p == '.')
			break;
		++p;
	}
	_prefix.assign (begin, p - begin);

	if (p == end )
		return true;

	if (*p != '*' && *p != '.')
	{
		return false;
	}
	// Star
	if (*p == '*')
	{
		_star1 = true;
		++p;
		if (p == end)
		{
			return true;
		}
	}
	// Dot
	if (*p == '.')
	{
		_dot = true;
		++p;
		if (p == end)
			return true;
	}
	// Prefix 2
	begin = p;
	while (p != end)
	{
		if (*p == '*')
		{
			break;
		}
		++p;
	}
	_ext.assign (begin, p - begin);
	// second star
	if (p != end)
	{
		Assert (*p == '*');
		_star2 = true;
		++p;
		if (p != end)
		{
			return false;
		}
	}
	return true;
}

bool FileMatcher::IsMatch (char const * name) const
{
	char const * begin = name;
	char const * end = name + strlen (name);

	if (!MatchPrefix1 (begin, end))
		return false;
	if (!MatchStar1 (begin, end))
		return false;
	if (!MatchDot (begin, end))
		return false;
	if (!MatchPrefix2 (begin, end))
		return false;
	if (!MatchStar2 (begin, end))
		return false;
	return true;
}

bool FileMatcher::MatchPrefix1 (char const* & begin, char const * end) const
{
	std::string::const_iterator it = _prefix.begin ();
	while (begin != end && it != _prefix.end ())
	{
		if (::ToLower (*begin) != ::ToLower (*it) && *it != '?')
			return false;
		++begin;
		++it;
	}
	return it == _prefix.end ();
}

bool FileMatcher::MatchPrefix2 (char const* & begin, char const * end) const
{
	std::string::const_iterator it = _ext.begin ();
	while (begin != end && it != _ext.end ())
	{
		if (::ToLower (*begin) != ::ToLower (*it) && *it != '?')
			return false;
		++begin;
		++it;
	}
	return it == _ext.end ();
}

bool FileMatcher::MatchStar1 (char const* & begin, char const * end) const
{
	if (_star1)
	{
		if (!_dot) // pattern ends with *
		{
			begin = end;
			return true;
		}

		while (begin != end)
		{
			if (*begin == '.')
				break;
			++begin;
		}
		return true;
	}
	else
	{
		return begin == end || *begin == '.';
	}
}

bool FileMatcher::MatchDot (char const* & begin, char const * end) const
{
	if (!_dot)
		return begin == end;
	else if (_ext.empty () && !_star2) // pattern ends with dot
		return begin == end;
	else if (_ext.empty () && _star2) // pattern end with .*
	{
		begin = end;
		return true;
	}
	else if (*begin == '.')
	{
		++begin;
		return true;
	}
	return false;
}

bool FileMatcher::MatchStar2 (char const* & begin, char const * end) const
{
	if (_star2)
		return true;
	else
		return begin == end;
}
