//-------------------------------------------------
//  IdeOptions.cpp
//  (c) Reliable Software, 2000
//-------------------------------------------------

#include "precompiled.h"
#include "IdeOptions.h"
#include "scc.h"

IdeOptions::IdeOptions (unsigned long value)
	: _value (0)
{
	if (value == SCC_KEEP_CHECKEDOUT)
		_bits._keepCheckedOut = 1;
	else if (value == SCC_FILETYPE_AUTO)
		_bits._autoFileType = 1;
	else if (value == SCC_FILETYPE_TEXT)
		_bits._text = 1;
	else if (value == SCC_FILETYPE_BINARY)
		_bits._binary = 1;
	else if (value == SCC_GET_ALL)
		_bits._all = 1;
	else if (value == SCC_GET_RECURSIVE)
		_bits._deep = 1;
}

