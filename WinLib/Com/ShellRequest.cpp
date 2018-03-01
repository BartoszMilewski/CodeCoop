//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include <WinLibBase.h>
#include "ShellRequest.h"

#include <File/File.h>
#include <File/ErrorLog.h>

using namespace ShellMan;

FileRequest::FileRequest ()
	: _flags (0)
{
	_flags = FOF_ALLOWUNDO;			// Preserve Undo information, if possible.
	_flags |= FOF_NOCONFIRMATION;	// Respond with Yes to All for any dialog box that is displayed.
}

void FileRequest::AddFile (char const * from)
{
	_from += from;
	_from += '\0';
}

void FileRequest::AddFile (std::string const & from)
{
	AddFile (from.c_str ());
}

void FileRequest::MakeReadWrite ()
{
	if (_from.empty ())
		return;
	_from += '\0';	// Make _from ended by double '\0';
	// _from contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	unsigned int i = 0;
	do
	{
		File::MakeReadWrite (&_from [i]);
		i = _from.find ('\0', i);
		i++;
	} while (_from [i] != '\0');
}

void FileRequest::MakeReadOnly ()
{
	if (_from.empty ())
		return;
	_from += '\0';	// Make _from ended by double '\0';
	// _from contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	unsigned int i = 0;
	do
	{
		File::MakeReadOnly (&_from [i]);
		i = _from.find ('\0', i);
		i++;
	} while (_from [i] != '\0');
}

void FileRequest::DoDelete (Win::Dow::Handle win, bool quiet)
{
	if (_from.empty ())
		return;
	_from += '\0';
	// ShellMan will place deleted files in the recycle bin
	ShellMan::DeleteFiles (win, _from.c_str (), _flags, "", quiet);
}

CopyRequest::CopyRequest (bool quiet)
{
	_flags = FOF_NOCONFIRMMKDIR;	//Does not confirm the creation of a New directory if the operation
									//requires one to be created.
	_flags |= FOF_ALLOWUNDO;		//Preserve Undo information, if possible.
	_flags |= FOF_NO_CONNECTED_ELEMENTS;//Do not move connected files as a group. Only move the specified files.
	if (quiet)
	{
		_flags |= FOF_SILENT;		// No UI
		_flags |= FOF_NOCONFIRMATION;//Respond with Yes to All for any dialog box that is displayed.
	}
	else
	{
		_flags |= FOF_SIMPLEPROGRESS;//Displays a progress dialog box but does not show the file names,
									//because we will use for source funny temprary area names.
	}
	_flags |= FOF_MULTIDESTFILES;	//Multiple destination files (one for each source file) rather
									//than one directory where all source files are to be deposited.
}

void CopyRequest::AddCopyRequest (char const * from, char const * to)
{
	FileRequest::AddFile (from);
	_to += to;
	_to += '\0';
}

void CopyRequest::DoCopy (Win::Dow::Handle win, char const * title)
{
	if (_from.empty ())
		return;
	_from += '\0';
	_to += '\0';
	// _to contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	int i = 0;
	do
	{
		if (File::Exists (&_to [i]))
			_willOverwrite.push_back (1);
		else
			_willOverwrite.push_back (0);
		i = _to.find ('\0', i);
		i++;
	} while (_to [i] != '\0');
	ShellMan::CopyFiles (win, _from.c_str (), _to.c_str (), _flags, title);
}

void CopyRequest::MakeDestinationReadWrite () const
{
	if (_to.empty ())
		return;
	// _to contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	int i = 0;
	do
	{
		File::MakeReadWrite (&_to [i]);
		i = _to.find ('\0', i);
		i++;
	} while (_to [i] != '\0');
}

void CopyRequest::MakeDestinationReadOnly () const
{
	if (_to.empty ())
		return;
	// _to contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	int i = 0;
	do
	{
		File::MakeReadOnly (&_to [i]);
		i = _to.find ('\0', i);
		i++;
	} while (_to [i] != '\0');
}

void CopyRequest::Cleanup () const
{
	if (_to.empty ())
		return;
	// _to contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	int i = 0;
	int j = 0;
	do
	{
		if (_willOverwrite [j] == 0)
			File::DeleteNoEx (&_to [i]);
		j++;
		i = _to.find ('\0', i);
		i++;
	} while (_to [i] != '\0');
}

#if !defined (NDEBUG)
void CopyRequest::Dump ()
{
	// _to contains multiple destination file paths separated
	// by '\0' and ended by double '\0'
	int idxFrom = 0;
	int idxTo = 0;
	TheErrorLog << "<font face=\"Courier\"><table border=1 cellpadding=4>";
	TheErrorLog << "<tr><th>From</th><th>To</th></tr>";
	std::string msg;
	do
	{
		msg.erase ();
		msg += "<tr><td>";
		int beg = idxFrom;
		idxFrom = _from.find ('\0', idxFrom) + 1;
		msg.append (_from, beg, idxFrom - beg - 1);
		msg += "</td><td>";
		beg = idxTo;
		idxTo = _to.find ('\0', idxTo) + 1;
		msg.append (_to, beg, idxTo - beg - 1);
		msg += "</td></tr>";
		TheErrorLog << msg.c_str ();
	} while (_to [idxTo] != '\0' && _from [idxFrom] != 0);
	TheErrorLog << "</table></font>";
	TheErrorLog << "Failed Copy Request";
}
#endif

