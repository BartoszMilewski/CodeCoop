#if !defined (WIKILEXER_H)
#define WIKILEXER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <iostream>

class WikiLexer
{
public:
	WikiLexer (std::istream & in)
		: _in (in), _cur (-1), _atEnd (false), _stopChar ('\0')
	{
		Accept ();
	}
	char Get () const { return AtEnd () ? '\0': _line [_cur]; }
	std::string const & GetLine () const { return _line; }
	void Accept ();
	void Accept (unsigned count);
	void SetStopChar (char c = '\0') { _stopChar = c; }
	bool HasStopChar () const { return _stopChar != '\0'; }
	void EatWhite ();
	void EatWhiteToEnd ();
	unsigned Count (char c);
	bool IsMatch (std::string const & pattern, bool strict = false);
	std::string GetString (char const stopChars []);
	std::string GetQuotedString ();

	bool AtEnd () const { return _atEnd; }
	bool IsNewLine () const { return _line [_cur] == '\n'; }
	bool IsStopChar () const { return _line [_cur] == _stopChar; }
	bool IsBreak () const { return AtEnd () | IsNewLine () | IsStopChar (); }
private:
	std::istream & _in;
	std::string _line;
	int			_cur;
	bool		_atEnd;
	char		_stopChar;
};

#endif
