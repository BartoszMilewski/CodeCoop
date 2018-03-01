#if !defined (RANDOMUNIQUENAME_H)
#define RANDOMUNIQUENAME_H
//---------------------------------------
//  RandomUniqueName.h
//  (c) Reliable Software, 2002
//---------------------------------------

class RandomUniqueName
{
public:
	RandomUniqueName ();
	RandomUniqueName (unsigned long value);
	RandomUniqueName (std::string const & str);

	unsigned long GetValue () const { return _value; }
	std::string const & GetString () const { return _valueStr; }

private:
	void FormatString ();

private:
	unsigned long	_value;
	std::string		_valueStr;
};

#endif
