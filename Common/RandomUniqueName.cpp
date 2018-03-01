//---------------------------------------
//  RandomUniqueName.cpp
//  (c) Reliable Software, 2002
//---------------------------------------

#include "precompiled.h"
#include "RandomUniqueName.h"

#include <sstream>
#include <iomanip>

RandomUniqueName::RandomUniqueName ()
{
	unsigned long v1 = rand ();
	unsigned long v2 = rand ();
	_value = (v1 << 16) | v2;
	FormatString ();
}

RandomUniqueName::RandomUniqueName (unsigned long value)
	: _value (value)
{
	FormatString ();
}

RandomUniqueName::RandomUniqueName (std::string const & str)
	: _valueStr (str)
{
	std::istringstream in (str);
	in >> std::hex >> _value;
}

void RandomUniqueName::FormatString ()
{
    std::ostringstream buffer;
	buffer << "0x" << std::hex << _value;
	_valueStr = buffer.str ();
}
