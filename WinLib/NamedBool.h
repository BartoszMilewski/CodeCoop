#if !defined (NAMEDBOOL_H)
#define NAMEDBOOL_H
//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------

class NamedBool : public std::set<std::string>
{
public:
	void Set (std::string const & name, bool val = true)
	{
		if (val == true)
			insert (name);
		else
			erase (name);
	}
	void ClearAll ()
	{
		clear ();
	}
	bool IsOn (std::string const & name) const
	{
		return find (name) != end ();
	}
};

#endif
