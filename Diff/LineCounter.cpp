//
// Reliable Software (c) 1998
//
#include "precompiled.h"
#include "LineCounter.h"

void LineCounterDiff::SetMode (EditStyle::Action act, int newLineNo)
{
	if (act == EditStyle::actDelete)
	{
		_doIncrement = false;
	}
	else
	{
		_doIncrement = true;
		_lineNo = newLineNo;
	}
}

int LineCounterDiff::NextLineNo ()
{
	if (_doIncrement)
		return _lineNo++;
	else
		return _lineNo;
}

void LineCounterMerge::SetMode (EditStyle::Action act, int newLineNo)
{
	if (act == EditStyle::actDelete || act == EditStyle::actCut)
	{
		_doIncrement = false;
	}
	else
	{
		_doIncrement = true;
	}
}

void LineCounterOrig::SetMode (EditStyle::Action act, int newLineNo)
{
	if (act == EditStyle::actNone || act == EditStyle::actDelete || act == EditStyle::actCut)
	{
		_doIncrement = true;
	}
	else
	{
		_doIncrement = false;
	}
}
