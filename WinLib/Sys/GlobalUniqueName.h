#if !defined (GLOBALUNIQUENAME_H)
#define GLOBALUNIQUENAME_H
//----------------------------------------------------
// (c) Reliable Software 2001 -- 2005
//----------------------------------------------------

class GlobalUniqueName
{
public:
	GlobalUniqueName ();
	GlobalUniqueName (GUID const & giud);

	std::string const & GetName () const { return _name; }

private:
	void Init (GUID const & guid);

private:
	std::string	_name;
};

#endif
