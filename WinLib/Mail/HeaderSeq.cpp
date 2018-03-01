// ---------------------------------
// (c) Reliable Software 2003 - 2005
// ---------------------------------
#include <WinLibBase.h>
#include "HeaderSeq.h"

void HeaderSeq::Advance ()
{
	// Empty line means end of headers
	if (_nextLine.size () == 0 || _lines.AtEnd ())
	{
		_done = true;
	}
	else
	{
		std::string foldedLine (_nextLine);
		_lines.Advance ();
		while (!_lines.AtEnd ())
		{
			// look at the beginning of the next line for 
			// the continuation of the current line
			_nextLine = _lines.Get ();
			if (_nextLine.size () > 0 && (_nextLine [0] == ' ' || _nextLine [0] == '\t'))
			{
				foldedLine.append (_nextLine);
				_lines.Advance ();
			}
			else
				break;
		}
		_comment.clear ();
		unsigned endPos = _namedAttribute.Init (foldedLine);
		if (endPos != std::string::npos)
		{
			Assert (endPos < foldedLine.size ());
			unsigned curPos = foldedLine.find_first_not_of (" \t", endPos);
			if (curPos != std::string::npos)
				_comment = foldedLine.substr (curPos);
		}
	}
}
