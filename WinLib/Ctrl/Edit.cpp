//-------------------------------------
// (c) Reliable Software 1998 -- 2004
//-------------------------------------

#include <WinLibBase.h>
#include "Edit.h"

#include "StringOp.h"

using namespace Win;

std::string Edit::GetString () const
{
	std::vector<char> buf;
	std::string result;
	int len = GetLen ();
	if (len > 0)
	{
		buf.resize (len + 1);
		GetString (&buf [0], len + 1);
		result.assign (&buf [0]);
	}
	return result;
}

std::string Edit::GetTrimmedString () const
{
	TrimmedString str (GetString ());
	return str;
}

bool Edit::GetInt (int & value)
{
	BOOL translated;
	unsigned val = ::GetDlgItemInt (GetParent ().ToNative (), GetId (), &translated, TRUE);
	value = static_cast<int> (val);
	return translated != FALSE;
}

bool Edit::GetUnsigned (unsigned & value)
{
	BOOL translated;
	value = ::GetDlgItemInt (GetParent ().ToNative (), GetId (), &translated, FALSE);
	return translated != FALSE;
}

void Edit::Append (char const * buf)
{
	int len = GetLen ();
	Select (len, -1);
	SendMsg (EM_REPLACESEL, 0, reinterpret_cast<LPARAM> (buf));
}

void Edit::SelectLine (int lineNo)
{
	int lineStart = SendMsg (EM_LINEINDEX, lineNo);
	int lineLen = SendMsg (EM_LINELENGTH, lineNo);
	Select (lineStart, lineStart + lineLen);
}

void Edit::Clear ()
{
	Select ();
	SendMsg (WM_CLEAR);
}

// ----------
// StreamEdit
// ----------

// Streambuf implementation

int Win::StreamEdit::sync ()
{
	// Important note:
	// Special treatment of line breaks:
	// Edit control considers CR+LF pair as line break
	// while LF is widely used, e.g. in standard library.
	// Solution: Replace all LFs with CR+LF pairs.
	char * bufEnd = pptr();
	if (bufEnd != 0)
	{
		// We've got data in the buffer
		*bufEnd = '\0';

		char * lineStart = _buffer;
		if (_buffer [0] == '\n') // '\n' at first position in the buffer
		{
			Append ("\r\n");
			++lineStart;
		}

		char * lineEnd = std::find (lineStart, bufEnd, '\n');
		if (lineEnd == bufEnd)
		{
			// no special characters found
			Append (lineStart);
		}
		else
		{
			do
			{
				Assert (lineEnd > _buffer && lineEnd < bufEnd);
				if (*(lineEnd - 1) != '\r')
				{
					// previous character is not CR
					std::string line (lineStart, lineEnd);
					line += "\r\n";
					Append (line);
					lineStart = lineEnd + 1;
				}
				lineEnd = std::find (lineEnd + 1, bufEnd, '\n');
			} while (lineEnd < bufEnd);
			std::string lastLine (lineStart, bufEnd);
			Append (lastLine);
		}
	}

	doallocate ();
	return 0;
}

int Win::StreamEdit::overflow (int nCh)
{
	sync ();
	if (nCh != EOF)
	{
		_buffer [0] = static_cast<char> (nCh);
		pbump (1);
	}
	return 0;
}

int Win::StreamEdit::doallocate ()
{
	setp (_buffer, _buffer + nPutAreaSize);
	return nPutAreaSize;
}
