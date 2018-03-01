// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include <WinLibBase.h>
#include "SmtpUtils.h"
#include <Net/Socket.h>
#include <sstream>

std::string ToBracketedString (std::string const & str)
{
	std::ostringstream bracketed;
	bracketed << '<' << str << '>';
	return bracketed.str ();
}

LineBreakingSeq::LineBreakingSeq (std::string const & text, unsigned int maxLineLength)
  : _text (text),
    _maxLineLength (maxLineLength),
	_isAddSpace (false),
	_currentIdx (0),
	_isAtEnd (false)
{
	Assert (maxLineLength > 1);
	if (_text.empty ())
	{
		_isAtEnd = true;
	}
	else
	{
		Advance ();
	}
}

void LineBreakingSeq::Advance ()
{
	Assert (!AtEnd ());

	if (_currentIdx >= _text.size ())
	{
		_isAtEnd = true;
		return;
	}

	_currentLine.resize (0);
	unsigned int endIdx = _currentIdx + _maxLineLength;
	if (_isAddSpace)
	{
		_currentLine += ' ';
		--endIdx;
		_isAddSpace = false;
	}
	if (endIdx > _text.size ())
		endIdx = _text.size ();
	
	_currentLine += _text.substr (_currentIdx, endIdx - _currentIdx);
	unsigned int const lineBreakPos = _currentLine.find ("\r\n");
	if (lineBreakPos == std::string::npos)
	{
		if (endIdx < _text.size ()) // there is a left-over
		{
			// line folding
			if (_text [endIdx] != ' ' && _text [endIdx] != '\t')
			{
				// need to break the line
				std::string::size_type wspPos = _currentLine.find_last_of (" \t");
				if (wspPos == std::string::npos || wspPos == 0)
				{
					// no good place to break the line;
					// our solution: insert a space by force
					_isAddSpace = true;
				}
				else
				{
					unsigned int delta = _currentLine.size () - wspPos;
					_currentLine.resize (wspPos);
					endIdx -= delta;
				}
			}
		}
	}
	else
	{
		endIdx -= _currentLine.size () - lineBreakPos - 2; // first position after CRLF
		_currentLine.resize (lineBreakPos);
	}
	if (_currentLine.size () == 1 && _currentLine [0] == '.')
	{
		// don't send a single dot on a line (SMTP end of section)
		_currentLine = "..";
	}
	_currentIdx = endIdx;
}

Smtp::Output::Output (Socket & socket)
: _socket (socket),
  _currLineLength (0)
{
	_currentLine [LineLength] = '\r';
	_currentLine [LineLength + 1] = '\n';
}

void Smtp::Output::Put (char c)
{
	Assert (_currLineLength < LineLength);
	Assert (_currLineLength != -1); // sanity: Flush was not called

	_currentLine [_currLineLength] = c;
	++_currLineLength;

	if (_currLineLength == LineLength)
	{
		// CRLF is preset at the end of line
		_socket.Send (&_currentLine [0], LineLengthWithCRLF);
		_currLineLength = 0;
	}
}

void Smtp::Output::Flush ()
{
	Assert (_currLineLength < LineLength);
	if (_currLineLength > 0)
	{
		_socket.Send (&_currentLine [0], _currLineLength);
		_currLineLength = -1;
	}
}

namespace UnitTest
{
	void SmtpLineBreaker (std::ostream & out)
	{
		out << std::endl << "Test of Smtp::LineBreakingSeq." << std::endl;
		// one-liners
		{
			std::string empty;
			LineBreakingSeq s (empty);
			Assert (s.AtEnd ());
		}
		{
			std::string oneliner ("One-liner");
			LineBreakingSeq s (oneliner, oneliner.size () + 1);
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == oneliner);
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			std::string oneliner ("One-liner");
			LineBreakingSeq s (oneliner, oneliner.size ());
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == oneliner);
			s.Advance ();
			Assert (s.AtEnd ());
		}
		// multi-liners
		{
			// general case
			std::string multiliner ("This is a multi-line string of characters to be broken into lines.");
			std::string outputLines [9] =
			{
				"This is a",
				" multi-lin",
				" e string",
				" of",
				" character",
				" s to be",
				" broken",
				" into",
				" lines."
			};
			unsigned int i = 0;
			for (LineBreakingSeq s (multiliner, 10); !s.AtEnd (); s.Advance ())
			{
				std::string currentLine = s.GetLine ();
				Assert (currentLine == outputLines [i]);
				++i;
			}
			Assert (i == 9);
		}
		{
			// general two-liner without whitespaces
			std::string twoliner ("Two-liner");
			LineBreakingSeq s (twoliner, twoliner.size () - 1);
			Assert (s.GetLine () == twoliner.substr (0, twoliner.size () - 1));
			s.Advance ();
			Assert (s.GetLine () == " r");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			// general two-liner with a whitespace
			std::string twoliner ("Two liner");
			LineBreakingSeq s (twoliner, twoliner.size () - 1);
			Assert (s.GetLine () == "Two");
			s.Advance ();
			Assert (s.GetLine () == " liner");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			std::string twoliner ("\r\nTwo-liner");
			LineBreakingSeq s (twoliner, twoliner.size ());
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == "");
			s.Advance ();
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == "Two-liner");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			std::string twoliner ("Two-liner\r\n");
			LineBreakingSeq s (twoliner, twoliner.size () - 2);
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == "Two-liner");
			s.Advance ();
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == " ");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			// whitespace at the first position after allowed line break
			std::string twoliner ("Two-liner ");
			LineBreakingSeq s (twoliner, twoliner.size () - 1);
			Assert (s.GetLine () == twoliner.substr (0, twoliner.size () - 1));
			s.Advance ();
			Assert (s.GetLine () == " ");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			// whitespace at the beginning of lines
			std::string twoliner (" Two-liner");
			LineBreakingSeq s (twoliner, twoliner.size () - 1);
			Assert (s.GetLine () == twoliner.substr (0, twoliner.size () - 1));
			s.Advance ();
			Assert (s.GetLine () == " r");
			s.Advance ();
			Assert (s.AtEnd ());
		}
		{
			// string without whitespaces
			std::string ten ("0123456789");
			std::string test;
			for (unsigned int i = 0; i < 5; ++i)
				test += ten;
			std::string outputLines [6] =
			{
				"0123456789",
				" 012345678",
				" 901234567",
				" 890123456",
				" 789012345",
				" 6789"
			};
			unsigned int j = 0;
			for (LineBreakingSeq s (test, ten.size ()); !s.AtEnd (); s.Advance ())
			{
				std::string output = s.GetLine ();
				out << j << " " << output.size () << " \"" << output << "\"" << std::endl;
				Assert (output == outputLines [j]);
				++j;
			}
			Assert (j == 6);
		}
		{
			// test minimal allowed line length of 2
			std::string test ("abcdefghijklmnoprstuwxyz");
			LineBreakingSeq s (test, 2);
			Assert (!s.AtEnd ());
			Assert (s.GetLine () == "ab");
			s.Advance ();
			Assert (!s.AtEnd ());
			for (; !s.AtEnd (); s.Advance ())
			{
				std::string output = s.GetLine ();
				Assert (output.size () == 2);
				Assert (output [0] == ' ');
				Assert (output [1] != ' ');
			}
		}
		{
			char const * inputLines [] = 
			{
				"This email confirms your order for a 5-seat license of Code Co-op v4.6 for",
				"$1. I have attached a hardcopy receipt to this email.",
				"",
				"License installation information:",
				"Download the latest version, 4.6, from ftp://ftp.relisoft.com/co-op46.exe.",
				"You can check your version from within the Code Co-op GUI by selecting",
				"Program > About. Install this exe over your existing installation of Code Co-op.",
				"You can check your current version by going to Program > About.",
				"Command-line applets can be downloaded from",
				"ftp://ftp.relisoft.com/cmdline46.exe.",
				"",
				"Input the license information exactly as shown below in each copy of Code Co-op.",
				"This is done by running Code Co-op and selecting Program and then License.",
				"This licensee name and alpha-numeric license key must be entered exactly",
				"as shown below.  This key will unlock the time limit and make you fully licensed",
				"for 5 workstations/enlistments.",
				"",
				"Licensee name: Bartosz",
				"License key:   xxxxxxxx",
				"If you have any questions or comments about Code Co-op, please contact us at",
				"support@relisoft.com or visit our online forum at",
				"http://www.relisoft.com/forums/",
				"",
				"Best regards,",
				"",
				"Debbie Ehrlich",
				"Reliable Software -- Home of Code Co-op, the peer-to-peer version control system",
				"www.relisoft.com",
				0
			};
			char const * resultLines [] = 
			{
				"This email confirms your order for a 5-seat license of Code Co-op v4.6 for",
				"$1. I have attached a hardcopy receipt to this email.",
				"",
				"License installation information:",
				"Download the latest version, 4.6, from ftp://ftp.relisoft.com/co-op46.exe.",
				"You can check your version from within the Code Co-op GUI by selecting",
				"Program > About. Install this exe over your existing installation of Code",
				" Co-op.",
				"You can check your current version by going to Program > About.",
				"Command-line applets can be downloaded from",
				"ftp://ftp.relisoft.com/cmdline46.exe.",
				"",
				"Input the license information exactly as shown below in each copy of Code",
				" Co-op.",
				"This is done by running Code Co-op and selecting Program and then License.",
				"This licensee name and alpha-numeric license key must be entered exactly",
				"as shown below.  This key will unlock the time limit and make you fully",
				" licensed",
				"for 5 workstations/enlistments.",
				"",
				"Licensee name: Bartosz",
				"License key:   xxxxxxxx",
				"If you have any questions or comments about Code Co-op, please contact us",
				" at",
				"support@relisoft.com or visit our online forum at",
				"http://www.relisoft.com/forums/",
				"",
				"Best regards,",
				"",
				"Debbie Ehrlich",
				"Reliable Software -- Home of Code Co-op, the peer-to-peer version control",
				" system",
				"www.relisoft.com",
				0
			};
			std::string multiliner;
			unsigned int lineCount = 0;
			for (unsigned int i = 0; inputLines [i] != 0; ++i)
			{
				multiliner += inputLines [i];
				multiliner += "\r\n"; 
				++lineCount;
			}
			unsigned int curr = 0;
			for (LineBreakingSeq s (multiliner); !s.AtEnd (); s.Advance ())
			{
				std::string currentLine = s.GetLine ();
				out << "\n" << currentLine;
				Assert (currentLine == resultLines [curr]);
				++curr;
			}
		}
		out << "\n\nPassed." << std::endl;
	}
}
