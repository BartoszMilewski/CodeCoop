//------------------------------------
// (c) Reliable Software, 1998 -- 2002
//------------------------------------

#include "precompiled.h"
#include "EditStyle.h"

EditStyle::Action EditStyle::GetAction () const
{
	if (_value == 0)
		return actNone;
	if (_bits._moved)
	{
		if (_bits._removed)
			return actCut;
		else
			return actPaste;
	}
	else
	{
		if (_bits._removed)
			return actDelete;
		else
			return actInsert;
	}
}

EditStyle::EditStyle (EditStyle::Source source, EditStyle::Action action)
{
	_bits._change = source;
	switch (action)
	{
    case actNone:
		_value = 0;
		break;
	case actDelete:
		_bits._moved = 0;
		_bits._removed = 1;
		break;
    case actInsert:
		_bits._moved = 0;
		_bits._removed = 0;
		break;
    case actCut:
		_bits._moved = 1;
		_bits._removed = 1;
		break;
    case actPaste:
		_bits._moved = 1;
		_bits._removed = 0;
		break;
	default:
		break;
	}
}
