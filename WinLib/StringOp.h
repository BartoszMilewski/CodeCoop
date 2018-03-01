#if !defined (STRINGOP_H)
#define STRINGOP_H
//----------------------------------
// (c) Reliable Software 1998 - 2006
// String operations and predicates
//----------------------------------

#include <sstream>
#include <cstring>
#include <cctype>
#include <locale>

// Convert any type understood by iostream to string
template<class T>
std::string ToString (T value)
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str ();
}

// Sequence of C-strings terminated by double '\0' characters
class MultiString
{
public:
	class const_iterator : public std::iterator<std::forward_iterator_tag, char>
	{
	public:
		const_iterator (std::string::const_iterator p)
			: _p (p)
		{}

		const_iterator operator ++();
		char const * operator *() const { return &(*_p); }
		bool operator != (const_iterator iter) { return _p != iter._p; }
		bool operator == (const_iterator iter) { return _p == iter._p; }
	private:
		std::string::const_iterator _p;
	};

public:
	typedef std::string value_type;

	MultiString ();
	explicit MultiString (std::string const & str);

	bool empty () const;
	void push_back (std::string const & str);

	const_iterator begin () const { return _str.begin (); }
	const_iterator end () const;

	std::string const & str () const { return _str; }

	void Assign (char const * chars, unsigned charsLen);

	typedef std::string & reference;
	typedef std::string const & const_reference;

private:
	void Validate ();

private:
	std::string	_str;
};

// To be used with APIs that take buffer and length
//	std::string result;
//	result.reserve (len);
//	CallAPI (writable_string (result), result.capacity ());
// Thanks to Greg Herlihy for this idea
template<class T>
class basic_writable_string
{
public:
	basic_writable_string (std::basic_string<T> & s)
	: _buf (s.capacity ()), _s (s)
	{
	}

	~basic_writable_string ()
	{
		// guaranteed not to throw!
		_s.assign (&_buf[0]);
	}

	operator T *() const { return get_buf (); }
	T * get_buf () const
	{
		return (T *) &_buf[0];
	}

private:
	std::vector<T>			_buf;
	std::basic_string<T> &	_s;
};

typedef basic_writable_string<char> writable_string;
typedef basic_writable_string<wchar_t> writable_wstring;

class Indentation
{
public:
	Indentation (unsigned nSpaces) : _nSpaces (nSpaces) {}
	unsigned NumSpaces () const { return _nSpaces; }
private:
	unsigned _nSpaces;
};

inline std::ostream & operator<< (std::ostream & out, Indentation const & spaces)
{
	if (spaces.NumSpaces () != 0)
	{
		std::string indent (spaces.NumSpaces (), ' ');
		out << indent;
	}
	return out;
}

std::string ToHexString (unsigned value);
std::string ToHexStr (char c);
std::wstring ToWString (std::string const & src);
std::string ToMBString (wchar_t const * wstr);

int ToInt (std::string const & str);
HWND ToHwnd(std::string const & str);
bool HexStrToUnsigned (char const * str, unsigned long & result);
bool StrToUnsigned (char const * str, unsigned long & result);

std::string FormatFileSize (long long size);

inline bool IsAscii (char c)
{
	return (c & 0x80) == 0;
}

inline bool IsSpace (char c)
{
	return (unsigned)(c + 1) <= 256 && std::isspace (c) != 0;
}

inline bool IsAlnum (char c)
{
	return (unsigned)(c + 1) <= 256 && std::isalnum (c) != 0;
}

inline bool IsAlpha (char c)
{
	return (unsigned)(c + 1) <= 256 && std::isalpha (c) != 0;
}

inline bool IsDigit (char c)
{
	return (unsigned)(c + 1) <= 256 && std::isdigit (c) != 0;
}

inline bool IsUpper (char c)
{
	return (unsigned)(c + 1) <= 256 && std::isupper (c) != 0;
}

inline char ToUpper (char c)
{
	return std::toupper (c);
}

inline char ToLower (char c)
{
	return std::tolower (c);
}

inline bool IsGraph (char c)
{
	return std::isgraph (c) != 0;
}

inline bool IsEndOfLine (char c)
{
	return c == '\n' || c == '\r';
}

inline bool IsWordBreak (char c) 
{ 
	return !IsAlnum (c) && c != '_';
}

template<class T>
void overlapped_copy (T * begin, T * end, T * target)
{
	while (begin != end)
	{
		*target++ = *begin++;
	}
}

bool HasGuidFormat (std::string const & str);

class TrimmedString: public std::string
{
public:
	TrimmedString ()
	{}
	TrimmedString (std::string const & str)
	{
		Init (str);
	}

	void Assign (std::string const & str)
	{
		Init (str);
	}
private:
	void Init (std::string const & str);
};

template<char C>
std::string ReplaceNullPadding (std::string const & str)
{
	std::string paddedStr (str);
	std::string::size_type paddingStart = paddedStr.find ('\0');
	if (paddingStart != std::string::npos)
	{
		std::string::size_type len = paddedStr.length () - paddingStart + 1;
		paddedStr.replace (paddingStart, len, len, C);
	}
	return paddedStr;
}

class CharSet
{
public:
	CharSet (char const * chars)
	{
		_set.insert (chars, chars + strlen (chars));
	}

	bool Contains (char c) const
	{
		return _set.find (c) != _set.end ();
	}

private:
	std::set<char>	_set;
};

//------------------
// String comparison
//------------------

// Optimization: facet toupper is virtual, we want it to be simple table lookup
class UpCaseTable
{
	class UpCaseChar
	{
	public:
		UpCaseChar (UpCaseTable const & table) : _table (&table) {}
		char operator () (char c)
		{
			return (*_table) [c];
		}
	private:
		UpCaseTable const * _table; // use pointer to make the predicate copyable
	};
public:
	// Empty string gets user's default locale
	UpCaseTable (char const * locName = "")
	{
		// initialize table
		for (int i = CHAR_MIN; i <= CHAR_MAX; ++i)
			_table [i - CHAR_MIN] = static_cast<char> (i);
		std::locale loc (locName);
		// character type facet
		std::ctype<char> const & ct = std::use_facet<std::ctype<char> >(loc);
		// up-case the whole table using this facet
		ct.toupper(_table, _table + (CHAR_MAX - CHAR_MIN + 1));
	}
	char operator [] (char c) const { return _table [static_cast<unsigned char>(c) - CHAR_MIN]; }
	void UpCaseString (std::string & str) const
	{
		std::transform (str.begin (), str.end (), str.begin (), UpCaseChar (*this));
	}
private:
	char _table [CHAR_MAX - CHAR_MIN + 1];
};

// Use global object, to make sure there's no race condition to initialize it
extern UpCaseTable const TheUpCaseTable;

// Predicate objects

class NocaseLessChar
{
public:
	NocaseLessChar (UpCaseTable const & table)
		: _table (&table)
	{}
	bool operator () (char c1, char c2)
	{
		return (*_table) [c1] < (*_table) [c2];
	}
private:
	UpCaseTable const * _table; // use pointer to make the predicate copyable
};

class NocaseEqualChar
{
public:
	NocaseEqualChar (UpCaseTable const & table)
		: _table (&table)
	{}
	bool operator () (char c1, char c2)
	{
		return (*_table) [c1] == (*_table) [c2];
	}
private:
	UpCaseTable const * _table; // use pointer to make the predicate copyable
};

// Use when first char already up-cased
class NocaseEqualChar2
{
public:
	NocaseEqualChar2 (UpCaseTable const & table)
		: _table (&table)
	{}
	bool operator () (char c1up, char c2)
	{
		return c1up == (*_table) [c2];
	}
private:
	UpCaseTable const * _table; // use pointer to make the predicate copyable
};

class NocaseLess : public std::binary_function<std::string const &, std::string const &, bool>
{
public:
	NocaseLess (UpCaseTable const & table = TheUpCaseTable)
		: _table (&table)
	{}
	bool operator () (std::string const & str1, std::string const & str2) const
	{
		return std::lexicographical_compare (
			str1.begin(), str1.end(),
			str2.begin(), str2.end(),
			NocaseLessChar (*_table));
	}
private:
	UpCaseTable const * _table; // use pointer to make the predicate copyable
};

// Predicate: case insensitive equality of strings
class NocaseEqual : public std::unary_function<std::string const &, bool>
{
public:
	NocaseEqual (std::string const & str, UpCaseTable const & table = TheUpCaseTable)
		: _str (str), _len (_str.size ()), _table (&table)
	{
		_table->UpCaseString (_str);
	}
	NocaseEqual (char const * str, UpCaseTable const & table = TheUpCaseTable)
		: _str (str), _len (_str.size ()), _table (&table)
	{
		_table->UpCaseString (_str);
	}
	bool operator() (std::string const & str) const
	{
		if (_len != str.size ())
			return false;
		return std::equal (_str.begin (), _str.end (), str.begin (), NocaseEqualChar2 (*_table));
	}
private:
	std::string			_str;
	size_t				_len;
	UpCaseTable const * _table; // use pointer to make the predicate copyable
};

// Predicate global functions

inline bool IsNocaseLess (std::string const & str1, std::string const & str2)
{
	return std::lexicographical_compare (
		str1.begin(), str1.end(),
		str2.begin(), str2.end(),
		NocaseLessChar (TheUpCaseTable));
}

inline bool IsNocaseEqual (std::string const & s1, std::string const & s2)
{
	if (s1.size () != s2.size ())
		return false;
	return std::equal (s1.begin (), s1.end (), s2.begin (), NocaseEqualChar (TheUpCaseTable));
}

inline bool IsNocaseEqual (std::string const & str1, unsigned int off1, 
						   std::string const & str2, unsigned int off2)
{
	if (str1.size () - off1 != str2.size () - off2)
		return false;
	return std::equal (str1.begin () + off1, str1.end (), 
		str2.begin () + off2, 
		NocaseEqualChar (TheUpCaseTable));
}

inline bool IsNocaseEqual (std::string const & str1, unsigned int off1, 
						   std::string const & str2, unsigned int off2, unsigned len)
{
	if (str1.size () - off1 < len || str2.size () - off2 < len)
		return false;
	return std::equal (str1.begin () + off1, str1.begin () + off1 + len, 
		str2.begin () + off2, 
		NocaseEqualChar (TheUpCaseTable));
}

inline int NocaseCompare (std::string const & s1, std::string const & s2)
{
	if (IsNocaseLess (s1, s2))
		return -1;
	if (IsNocaseLess (s2, s1))
		return 1;
	return 0;
}

inline bool IsCaseEqual (std::string const & s1, std::string const & s2)
{
	return s1 == s2;
}

unsigned NocaseContains (std::string const & s1, std::string const & s2);

unsigned NocaseFind (std::string const & s1, unsigned off1, std::string const & s2, unsigned off2, unsigned len);

// File name comparison: Simple implementation based on no-case comparison
class NocaseNameCmp
{
public:
	bool operator () (char const * name1, char const * name2) const
	{
		return IsNocaseLess (name1, name2);
	}
};

// Case insensitive string map parametrized by value type

template<class T>
class NocaseMap: public std::map<std::string, T, NocaseLess>
{};

// Case insensitive string set

class NocaseSet: public std::set<std::string, NocaseLess>
{};

// File name comparing

inline bool IsFileNameEqual (std::string const & s1, std::string const & s2)
{
	return IsNocaseEqual (s1, s2);
}

inline bool IsFileNameLess (std::string const & s1, std::string const & s2)
{
	return IsNocaseLess (s1, s2);
}

inline int FileNameCompare (std::string const & s1, std::string const & s2)
{
	return NocaseCompare (s1, s2);
}

class FileNameLess : public NocaseLess
{
public:
	FileNameLess (UpCaseTable const & table = TheUpCaseTable) 
		: NocaseLess (table)
	{}
};

class FileNameEqual : public NocaseEqual
{
public:
	FileNameEqual (std::string const & str, UpCaseTable const & table = TheUpCaseTable)
		: NocaseEqual (str, table)
	{}
	FileNameEqual (char const * str, UpCaseTable const & table = TheUpCaseTable)
		: NocaseEqual (str, table)
	{}
};

class NamedValues
{
public:
	typedef std::map<std::string, std::string>::const_iterator Iterator;
public:
	std::string GetValue (std::string const & name) const;
	void Add (std::string const & name, std::string const & value)
	{
		_map [name] = value;
	}
	void Clear ()
	{
		_map.clear ();
	}
	Iterator begin () const { return _map.begin (); }
	Iterator end () const { return _map.end (); }
	unsigned size () const { return _map.size (); }
private:
	std::map<std::string, std::string> _map;
};

#endif
