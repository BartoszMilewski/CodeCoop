//
// (c) Reliable Software, 1997
//

#include "precompiled.h"
#include "Caret.h"
#include "Mapper.h"

void Caret::Create ()
{
	Win::Caret::Create (2, _mapper.LineHeight ());
}

void Caret::Position (int col, int line)
{
	Win::Point p (_mapper.ColToX (col), _mapper.LineToY (line));
	Win::Caret::SetPosition (p);
}

void Caret::GetPosition (int & col, int & line) const
{
	Win::Point p;
	Win::Caret::GetPosition (p);
	col = _mapper.XtoColTrunc (p.x);
	line = _mapper.YtoLineTrunc (p.y);
}
