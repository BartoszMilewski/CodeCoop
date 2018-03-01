// -------------------------------
// (c) Reliable Software 2003 - 05
// -------------------------------
#include <WinLibBase.h>
#include "BufferedStream.h"

LineBreaker::LineBreaker (BufferedStream & stream)
: _stream (stream)
{
	Advance ();
}

void LineBreaker::Advance ()
{
	_line.clear ();
	char c = _stream.GetChar ();
	for (;;)
	{
		// End of line?
		if (c == '\r' && _stream.LookAhead () == '\n')
		{
			// eat LF
			_stream.GetChar ();
			break;
		}
		_line += c;
		c = _stream.GetChar ();
	}
}
