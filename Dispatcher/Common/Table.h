#if !defined (TABLE_H)
#define TABLE_H
//-------------------------------------
// (c) Reliable Software 1998 -- 2002
// ------------------------------------

#include <Dbg/Assert.h>

//
// Table -- data source for display
//

class Restriction
{
public:
	Restriction () 
		: _number (-1), 
		  _isStrSet (false) 
	{}
	Restriction (int number) 
		: _number (number), 
		  _isStrSet (false) 
	{}
	Restriction (std::string const & str) 
		: _number (-1), 
		  _str (str), 
		  _isStrSet (true) 
	{}
	bool IsEqual (Restriction const & restrict) const
	{
		return _number == restrict._number &&  
			   _str == restrict._str && 
			   _isStrSet == restrict._isStrSet;
	}
	int GetNumber () const
	{
		Assert (IsNumberSet ());
		return _number;
	}
	std::string const & GetString () const
	{
		Assert (IsStringSet ());
		return _str;
	}
	bool IsNumberSet () const { return _number != -1; }
	bool IsStringSet () const { return _isStrSet; }
private:
	int			_number;
	std::string _str;
	bool		_isStrSet;
};

class TripleKey
{
public:
	bool IsEqual (TripleKey const & key) const
	{
		return _str1 == key._str1 &&
			   _str2 == key._str2 &&
			   _flag == key._flag;
	}
protected:
	std::string _str1;
	std::string _str2;
	bool _flag;
};

class Table
{
public:
    enum Column
    {
        colProjName,    // char const *
        colProjId,      // int
		colScriptCount, // int
		colStatus,		// int
		colPath,		// char const *
		colComment,		// char const *
		colMembers,		// int
		colHubId,		// std::string
		colRoute,		// std::string
		colMethod,      // int
		colDate,        // std::string
		colCount		// int
    };

public:
	virtual ~Table () {}

    virtual void QueryUniqueIds   (std::vector<int>& uids, Restriction const * restrict = 0) {};
    virtual void QueryUniqueNames (std::vector<std::string> & unames,
								   Restriction const * restrict = 0) {};
    virtual void QueryUniqueTripleKeys (std::vector<TripleKey> & ukeys,
							  		    Restriction const * restrict = 0) {};

	virtual std::string QueryCaption (Restriction const & r) const 
	{
		Assert (!"Method of abstract class called"); 
		return std::string ();
	}

    virtual std::string	GetStringField (Column col, int uid) const { return std::string (); }
    virtual int	GetNumericField (Column col, int uid) const { return 0; }

    virtual std::string	GetStringField (Column col, std::string const & uname) const { return std::string (); }
    virtual int	GetNumericField (Column col, std::string const & uname) const { return 0; }

    virtual std::string	GetStringField (Column col, TripleKey const & ukey) const { return std::string (); }
    virtual int	GetNumericField (Column col, TripleKey const & ukey) const { return 0; }

protected:
	Table () : _restriction (0) {}
	// current restriction
	// it may be set by QueryUniqueIds/Names
	// table uses it until the next call to QueryUniqueIds/Names 
	// which resets the restriction
	Restriction const * _restriction;
};

class RecordSet;

class TableProvider
{
public:
	virtual ~TableProvider () {}

    virtual std::unique_ptr<RecordSet> Query (std::string const & name, Restriction const & restrict) = 0;
	virtual std::string QueryCaption (std::string const & tableName, Restriction const & restrict) const = 0;
};

#endif
