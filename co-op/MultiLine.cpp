//---------------------------------------
//  (c) Reliable Software, 2003
//---------------------------------------

#include "precompiled.h"
#include "MultiLine.h"

#include <Dbg/Assert.h>

#include <StringOp.h>

MultiLineComment::MultiLineComment (std::string const & comment, unsigned int maxLineLength)
{
	Init (comment, maxLineLength);
}

void MultiLineComment::Init (std::string const & comment, unsigned int maxLineLength)
{
    // Break comment into lines
	std::string::size_type startPos = 0;
	std::string::size_type endPos = 0;
	_lines.clear ();
    do
    {
		std::string line;
		endPos = comment.find_first_of ("\r\n", startPos);
		if (endPos == std::string::npos)
		{
			// New line not found
			line = comment.substr (startPos);
		}
		else
		{
			// Found new line
			Assert (startPos <= endPos);
			line = comment.substr (startPos, endPos - startPos);
			startPos = endPos + 1;
			while (startPos < comment.length () && IsEndOfLine (comment [startPos]))
				startPos += 1;
		}
		// Skip empty lines at the comment begin
		if (line.length () == 0 && _lines.size () == 0)
			continue;
		if (line.length () < maxLineLength)
		{
			_lines.push_back (line);
		}
		else
		{
			// Split line into segments not longer then maxLineLength.
			// Break line at white space.
			std::string::size_type segmentStart = 0;
			std::string::size_type segmentEnd;
			std::string::size_type searchStart = 1; 
			do
			{
				searchStart += maxLineLength;
				std::string segment;
				if (searchStart < line.length ())
				{
					segmentEnd = line.rfind (' ', searchStart);
					if (segmentStart < segmentEnd)
					{
						// White space found between segment start and segment end
						segment = line.substr (segmentStart, segmentEnd - segmentStart);
						segmentStart = segmentEnd + 1;
					}
					else if (segmentEnd == std::string::npos)
					{
						// No white space found -- split into two segments
						_lines.push_back (line.substr (0, maxLineLength));
						segment = line.substr (maxLineLength + 1);
						searchStart = line.length () + 1;
					}
					else
					{
						// In the last segment no white space found
						Assert (segmentStart >= segmentEnd);
						segment = line.substr (segmentStart);
					}
				}
				else
				{
					segment = line.substr (segmentStart);
				}
				_lines.push_back (segment);
			}
			while (searchStart < line.length ());
		}
    }
	while (startPos < comment.length () && endPos != std::string::npos);
}
