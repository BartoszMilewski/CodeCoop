//-------------------------------------
// (c) Reliable Software 1998-2003
//-------------------------------------
#include "Edit.h"

using namespace Win;

std::string Edit::GetString () const
{
	std::string buf;
	int len = GetLen ();
	if (len > 0)
	{
		buf.resize (len);
		GetString (&buf [0], len + 1);
	}
	return buf;
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
