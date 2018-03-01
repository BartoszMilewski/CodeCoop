#if !defined (NAMEDPAIR_H)
#define NAMEDPAIR_H
// (c) Reliable Software 2003

// Parse name+delimiter+value+endDelimiter (strip quotes from value)

template<char delimiter, char endDelimiter = '\0', char quotMark = '"'>
class NamedPair
{
public:
	NamedPair () {}
	NamedPair (std::string const & str, unsigned startPos = 0)
	{
		Init (str, startPos);
	}
	unsigned Init (std::string const & str, unsigned startPos = 0);
	void Clear ()
	{
		_name.clear ();
		_value.clear ();
	}
	std::string const & GetName () const { return _name; }
	std::string const & GetValue () const { return _value; }
private:
	std::string	_name;
	std::string _value;
};

// <'='> foo= "bar"
// <':'> foo :bar
// <'=', ';'> foo = bar ;
// returns the position after the endDelimiter or std::string::npos

// Revisit:
// return bool : found pair or not, 
// pass another param: endPos -- if success: first position after the end of pair, else undefined
// use TrimmedString class
// write unit test
template<char delimiter, char endDelimiter, char quotMark>
unsigned NamedPair<delimiter, endDelimiter, quotMark>::Init (std::string const & str, unsigned startPos)
{
	unsigned curPos = str.find_first_not_of (" \t", startPos); // skip space
	unsigned endPos = str.find (delimiter, curPos); // find delimiter
	if (endPos == std::string::npos)
	{
		_name.clear ();
		_value.clear ();
		return std::string::npos; // delimiter not found
	}
	_name = str.substr (curPos, endPos - curPos);
	// trim trailing whitespace
	while (_name.size () > 0 && (_name [_name.size () - 1] == ' ' || _name [_name.size () - 1] == '\t'))
		_name.resize (_name.size () - 1);
	curPos = str.find_first_not_of (" \t", endPos + 1); // skip space after delimiter
	if (curPos == std::string::npos)
	{
		// No value found!
		_value.clear ();
		return std::string::npos;
	}

	if (str [curPos] == quotMark)
	{
		++curPos;
		// quoted string: find closing quote
		endPos = str.find (quotMark, curPos);
		if (endPos == std::string::npos) // closing quote not found
		{
			_value = str.substr (curPos);
			return std::string::npos;
		}
		_value = str.substr (curPos, endPos - curPos);
		endPos = str.find (endDelimiter, endPos + 1);
		return endPos == std::string::npos? std::string::npos: endPos + 1;
	}
	// no quotes
	endPos = str.find (endDelimiter, curPos); // find end delimiter
	unsigned endDelimiterPos = endPos;
	if (endPos == std::string::npos)
		endPos = str.size ();
	// eat trailing whitespace
	while (endPos > curPos && (str [endPos - 1] == ' ' || str [endPos - 1] == '\t'))
		--endPos;
	if (curPos == endPos)
	{
		_value.clear ();
		return std::string::npos;
	}
	_value = str.substr (curPos, endPos - curPos);
	if (endDelimiterPos == std::string::npos ||
		endDelimiterPos == str.size () - 1)
	{
		return std::string::npos;
	}
	else
		return endDelimiterPos + 1;
}

#endif
