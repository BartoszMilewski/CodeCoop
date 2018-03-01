//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include "precompiled.h"

#include "SqlNode.h"

// Match patterns of the form "%ab%cd%", or "ab%cd%ef", etc.
// Where '%' is a wildcard--matches 0 or more characters.
// Empty pattern matches only empty (or nonexisting) property
// "%" matches everything, including nonexistent property
bool Sql::LikeNode::Match (TuplesMap const & tuples) const
{
	TuplesMap::const_iterator it = tuples.find (_prop);
	if (it == tuples.end ())
		return _value.empty () || _value == "%";
	if (_value.empty ())
		return false;
	if (_value == "%")
		return true;
	std::string const & propValue = it->second;
	unsigned curStr = 0;
	unsigned curPat = 0;
	unsigned endPat = _value.find ('%');
	if (endPat == std::string::npos)
		endPat = _value.size ();
	if (endPat == 0)
	{
		// pattern starts with %
		Assert (_value.size () > 1);
		++curPat;
	}
	else
	{
		// first segment must match the beginning of the string
		if (endPat == std::string::npos)
			endPat = _value.size ();
		if (!IsNocaseEqual (propValue, 0, _value, 0, endPat))
			return false;
		if (endPat == _value.size ()) // pattern consumed
			return propValue.size () == _value.size ();
		curPat = endPat + 1;
	}

	return MatchWildcard (propValue, curStr, curPat);
}

bool Sql::LikeNode::MatchWildcard (std::string const & str, unsigned curStr, unsigned curPat) const
{
	Assert (_value [curPat - 1] == '%');
	if (curPat == _value.size ()) // It's a trailing %
		return true;
	unsigned endPat = _value.find ('%', curPat);
	if (endPat == std::string::npos)
	{
		// match exactly the end of string
		unsigned patLen = _value.size () - curPat;
		if (patLen > str.size () - curStr)
			return false;

		return IsNocaseEqual (str, str.size () - patLen, _value, curPat, patLen);
	}

	curStr = NocaseFind (str, curStr, _value, curPat, endPat - curPat);
	if (curStr == std::string::npos)
		return false;
	// we matched the pattern segment starting at curPat.
	// endPat points at the next %
	curStr += endPat - curPat;
	curPat = endPat + 1;
	return MatchWildcard (str, curStr, curPat);
}

// characters belonging to a word are either alphanumeric or non-ascii
// non-ascii are probably additional letters in non-English alphabets)
inline bool IsBreak (char c)
{
	return !IsAlnum (c) && IsAscii (c);
}

void WordBreak (std::string const & str, std::vector<std::string> & phrase)
{
	unsigned len = str.size ();
	unsigned beg = 0;
	unsigned end = 0;
	while (end != len)
	{
		char c = str [end];
		if (IsBreak (c))
		{
			phrase.push_back (str.substr (beg, end - beg));
			++end;
			while (end != len && IsBreak (str [end]))
			{
				++end;
			}
			if (end == len)
				return;
			beg = end;
		}
		++end;
	}
	phrase.push_back (str.substr (beg, end - beg));
}

bool Sql::ContainsNode::Match (TuplesMap const & tuples) const
{
	if (_value.empty ())
		return true;
	TuplesMap::const_iterator it = tuples.find (_prop);
	if (it == tuples.end ())
		return false;
	typedef std::vector<std::string> Phrase;
	Phrase textPhrase;
	WordBreak (it->second, textPhrase);
	Phrase patPhrase;
	WordBreak (_value, patPhrase);
	Phrase::const_iterator pit = patPhrase.begin ();
	std::string const & firstWord = *pit;
	++pit;
	for (Phrase::const_iterator it = textPhrase.begin (); it != textPhrase.end (); ++it)
	{
		if (IsNocaseEqual (*it, firstWord))
		{
			Phrase::const_iterator tmpIt = it;
			++tmpIt;
			Phrase::const_iterator tmpPit = pit;
			while (tmpPit != patPhrase.end () && tmpIt != textPhrase.end ())
			{
				if (!IsNocaseEqual (*tmpIt, *tmpPit))
					break;
				++tmpPit;
				++tmpIt;
			}
			if (tmpPit == patPhrase.end ())
				return true;
			if (tmpIt == textPhrase.end ())
				return false;
		}
	}
	return false;
}

bool Sql::InNode::Match (TuplesMap const & tuples) const
{
	if (_values.empty ())
		return false;
	TuplesMap::const_iterator it = tuples.find (_prop);
	if (it == tuples.end ())
		return false;
	return _values.find (it->second) != _values.end ();
}

void Sql::InNode::Dump (std::ostream & out, int indent) const
{
	Indent (out, indent);
	out << _prop << "IN ( ";
	for (NocaseSet::const_iterator it = _values.begin (); it != _values.end (); ++it)
		out << *it << " ";
	out << ")" << std::endl;
}
