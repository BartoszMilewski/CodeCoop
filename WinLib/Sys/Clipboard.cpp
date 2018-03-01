//----------------------------------------------------
// Clipboard.cpp
// (c) Reliable Software 2001 -- 2003
//
//----------------------------------------------------

#include <WinLibBase.h>
#include "Clipboard.h"
#include "WinGlobalMem.h"

void Clipboard::Clear ()
{
	Lock lock (_winOwner);
	::EmptyClipboard ();
}

bool Clipboard::HasText () const
{
	if (IsFormatText ())
	{
		GlobalBuf buf (GetText ());
		return strlen (&buf [0]) != 0 ? true : false;
	}
	return false;
}

void Clipboard::PutText (char const * text, int len)
{
	// Allocate global memory
	GlobalMem mem (len + 1);

	{
		GlobalBuf globalBuf (mem);
		memcpy (&globalBuf [0], text, len);
	}
	Lock lock (_winOwner);
	::EmptyClipboard ();
	::SetClipboardData (CF_TEXT, mem.Acquire ());
}

HGLOBAL Clipboard::GetText () const
{
	Lock lock (_winOwner);
	HGLOBAL handle = ::GetClipboardData (CF_TEXT);
	if (handle == 0)
		throw Win::Exception ("Internal error: Cannot retrieve text from the clipboard");
	return handle;
}

HDROP Clipboard::GetFileDrop () const
{
	Lock lock (_winOwner);
	HGLOBAL handle = ::GetClipboardData (CF_HDROP);
	if (handle == 0)
		throw Win::Exception ("Internal error: Cannot retrieve file(s) from the clipboard");
	return reinterpret_cast<HDROP>(handle);
}

void Clipboard::PutFileDrop (std::vector<std::string> const & files)
{
	GlobalMem dropFilesMem;
	MakeFileDropPackage (files, dropFilesMem);
	Lock lock (_winOwner);
	::EmptyClipboard ();
	::SetClipboardData (CF_HDROP, dropFilesMem.Acquire ());
}

void Clipboard::MakeFileDropPackage (std::vector<std::string> const & files, GlobalMem & package)
{
	// Prepare DROPFILES
	unsigned int size = sizeof (DROPFILES);
	std::vector<std::string>::const_iterator iter;
	for (iter = files.begin (); iter != files.end (); ++iter)
	{
		std::string const & filePath = *iter;
		size += filePath.length () + 1;
	}
	++size;	// For the double null termination
	package.Allocate (size);
	{
		GlobalBuf buf (package);
		DROPFILES * dropFiles = reinterpret_cast<DROPFILES *>(&buf [0]);
		dropFiles->pFiles = sizeof (DROPFILES);
		dropFiles->pt.x = -1;
		dropFiles->pt.y = -1;
		dropFiles->fNC = FALSE;
		dropFiles->fWide = FALSE;
		char * tmp = reinterpret_cast<char *>(&buf [sizeof (DROPFILES)]);
		// Copy file paths
		for (iter = files.begin (); iter != files.end (); ++iter)
		{
			std::string const & filePath = *iter;
			strcpy (tmp, filePath.c_str ());
			tmp += filePath.length () + 1;
		}
		*tmp = '\0';
	}
}
