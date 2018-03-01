//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include <WinLibBase.h>

#include "Dir.h"

FileSeq::FileSeq (char const * pattern)
	:_atEnd (true), _handle (INVALID_HANDLE_VALUE)
{
	if (pattern != 0)
		Open (pattern);
}

FileSeq::FileSeq (std::string const & pattern)
	:_atEnd (true), _handle (INVALID_HANDLE_VALUE)
{
	if (pattern.size () != 0)
		Open (pattern.c_str ());
}

void FileSeq::Open (char const * pattern)
{
	Assert (_handle == INVALID_HANDLE_VALUE);
	_handle = ::FindFirstFile (pattern, &_data);
    if (_handle == INVALID_HANDLE_VALUE)
    {
        int err = ::GetLastError ();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
		{
            _atEnd = true;
			Win::ClearError ();
		}
        else
        {
            throw Win::Exception ("Cannot list files.", pattern);
        }
    }
	else
		_atEnd = false;
	// skip "." and ".." directories
    while (!_atEnd && IsFolder () && IsDots ())
	{
		_atEnd = !::FindNextFile (_handle, &_data);
		if (_atEnd)
			Win::ClearError ();
	}
}

void FileSeq::Close ()
{
	if (_handle != INVALID_HANDLE_VALUE)
		::FindClose (_handle);
	_handle = INVALID_HANDLE_VALUE;
	_atEnd = true;
}

void FileSeq::Advance ()
{
	_atEnd = !::FindNextFile (_handle, &_data);
	if (_atEnd)
		Win::ClearError ();
}

bool FileSeq::AtEnd () const
{
    return _atEnd;
}

char const * FileSeq::GetName () const
{
	Assert (!_atEnd);
    return _data.cFileName;
}

FileMultiSeq::FileMultiSeq (FilePath const & dir, FileMultiSeq::Patterns const & patterns)
: _dir (dir.GetDir ()), _curPattern (patterns.begin ()), _endPattern (patterns.end ())
{
	Init ();
}

FileMultiSeq::FileMultiSeq (File::Vpath const & dir, FileMultiSeq::Patterns const & patterns)
: _dir (dir), _curPattern (patterns.begin ()), _endPattern (patterns.end ())
{
	Init ();
}

void FileMultiSeq::Init ()
{
	std::string path = _dir.GetFilePath (*_curPattern);
	FileSeq::Open (path.c_str ());
	if (FileSeq::AtEnd ())
		NextPattern ();
}

void FileMultiSeq::Advance ()
{
	FileSeq::Advance ();
	if (FileSeq::AtEnd ())
		NextPattern ();
}

void FileMultiSeq::NextPattern ()
{
	Assert (FileSeq::AtEnd ());
	do
	{
		FileSeq::Close ();
		++_curPattern;
		if (_curPattern != _endPattern)
		{
			std::string path = _dir.GetFilePath (*_curPattern);
			FileSeq::Open (path.c_str ());
		}
		else
			break;
	} while (FileSeq::AtEnd ());
}

DirSeq::DirSeq (char const *pattern)
    : FileSeq (pattern)
{
	// skip non-directories
	while (!AtEnd () && !IsFolder ())
		FileSeq::Advance ();
}

void DirSeq::Advance ()
{
	Assert (!AtEnd ());
	// advance and skip non-directories
    do
    {
        FileSeq::Advance ();
        if (AtEnd () || IsFolder ())
			break;
    } while (true);
}
