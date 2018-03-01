#if !defined (CMDLINESCANNER_H)
#define CMDLINESCANNER_H
//-----------------------------------------
//  CmdLineScanner.h
//  (c) Reliable Software, 1999, 2000
//-----------------------------------------

class CmdLineScanner
{
public:
	enum Token
	{
		tokSwitch,
		tokColon,
		tokString,
		tokEnd
	};

public:
	CmdLineScanner (char const * str = 0)
		: _str (str), _cur (0)
	{
		Accept ();
	}
	void Init (std::string & str)
	{
		_str = str;
		_cur = 0;
		Accept ();
	}
	Token Look () const { return _token; }
	std::string GetString ();
	void Accept ();

private:
	bool EatWhite ();
	static char const * const whiteChars;
	static char const * const stopChars;

	unsigned int	_cur;
	unsigned int	_begString;
	unsigned int	_endString;
	std::string		_str;
	Token			_token;
};

#endif
