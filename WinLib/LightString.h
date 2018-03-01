#if !defined LIGHTSTRING_H
#define LIGHTSTRING_H

//-----------------------------------------
// (c) Reliable Software 1997 -- 2003
//-----------------------------------------

#include <sstream>

//
// Formatted Message
//

class Msg : public std::ostringstream
{
public:
	char const * c_str () const
	{
		_str.assign (str ());
		return _str.c_str ();
	}
	operator std::string const & () const
	{
		_str.assign (str ());
		return _str;
	}
	void Clear () {	str (""); }
	bool IsEmpty () const { return str ().empty (); }

private:
	std::string	mutable _str;
};

#endif
