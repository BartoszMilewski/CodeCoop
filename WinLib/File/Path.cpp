//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "Path.h"
#include "File.h"
#include "Dir.h"

#include <Sys/RegKey.h>
#include <StringOp.h>

char const FilePath::IllegalChars [] = "<>\"\n\r\t*?";

char const FilePath::_allFilesName [] = "*.*";
char const FilePath::_pathSeparators [] = "\\/";
char const FilePath::_emptyPath [] = "";

//-----------PREDICATES---------------------------

// predicate for path character comparison
inline bool PathCharLess (char a, char b)
{
	if (a == b) 
		return false; // common case
	// separator always < non-separator
	if (FilePath::IsSeparator (a))
		return !FilePath::IsSeparator (b);
	if (FilePath::IsSeparator (b))
		return false; // non-separator > separator
	return ::ToLower (a) < ::ToLower (b);
}

//-------------------------------------------------

void FilePath::Clear ()
{
	//	swapping with local variable will cause any memory associated with _buf
	//	to be deallocated immediately
	std::string empty;
	_buf.swap (empty);
	_prefix = 0;
}

void FilePath::Change (char const * newPath)
{
	_buf.assign (newPath);
	if (!_buf.empty ())
	{
		// Remove terminating backslash, if present
		std::string::size_type lastPos = _buf.length () - 1;
		if (FilePath::IsSeparator (_buf [lastPos]))
			_buf.resize (lastPos);
	}
	_prefix = _buf.length ();
}

void FilePath::Change (FilePath const & newPath)
{
	Change (newPath.GetDir ());
}

void FilePath::Change (std::string const & newPath)
{
	Change (newPath.c_str ());
}

void FilePath::DirUp ()
{
	size_t backSlashPos = _buf.find_last_of (_pathSeparators, _prefix - 1);
	if (backSlashPos != std::string::npos)
	{
		_buf.resize (backSlashPos);
		_prefix = _buf.length ();
	}
	else
	{
		_buf.clear ();
		_prefix = 0;
	}
}

// Append sub folder to the current path; move _prefix
void FilePath::DirDown (char const * subDir, bool useSlash)
{
	if (_prefix > 0 && IsSeparator (_buf [_prefix - 1]))
		--_prefix;
	_buf.resize (_prefix);
	Append (subDir, useSlash);
	_prefix = _buf.length ();
}

bool FilePath::DirDownMaterialize(char const * subDir, bool quiet)
{
	DirDown(subDir);
	char const * path = GetDir ();
	if (!File::Exists (path))
	{
		// If not quiet - throw exception if cannot create folder
		if (!File::CreateFolder (path, quiet))
			return false;
	}
	return true;
}

char const * FilePath::GetAllFilesPath () const
{
	return GetFilePath (_allFilesName);
}

char const * FilePath::GetDir () const
{
	_buf.resize (_prefix);
	if (_prefix == 2 && _buf [_prefix - 1] == ':')
	{
		// Handle special case "c:"
		_buf.append (1, _backSlash);
	}
	return _buf.c_str ();
}

// Extract relative path at prefix
char const * FilePath::GetRelativePath (unsigned int prefix) const
{
	char const * fullPath = GetDir ();
	if (_buf.length () > prefix)
		return &fullPath [prefix];
	else
		return _emptyPath;
}

// Append sub folder to the current path; don't move _prefix
void FilePath::Append (std::string const & subDir, bool useSlash)
{
	Assert (subDir.length () != 0);

	if (_buf.empty ())
	{
		_buf.assign (subDir);
	}
	else
	{
		_buf.append (1, useSlash? _forwardSlash: _backSlash);
		_buf.append (subDir);
	}
}

std::string FilePath::GetDrive () const
{
	Assert (IsAbsolute (_buf));
	FullPathSeq pathSeq (_buf.c_str ());
	return pathSeq.GetHead ();
}

// File full path -- append file name to the current path at _prefix
char const * FilePath::GetFilePath (char const * fileName, bool useSlash) const
{
	_buf.resize (_prefix);
	if (!_buf.empty () 
		&& !FilePath::IsSeparator (fileName [0]) 
		&& !FilePath::IsSeparator (_buf [_buf.size () - 1]))
	{
		_buf.append (1, useSlash? _forwardSlash: _backSlash);
	}
	_buf.append (fileName);
	return _buf.c_str ();
}

char const * FilePath::GetFilePath (std::string const & fileName, bool useSlash) const
{
	return GetFilePath (fileName.c_str (), useSlash);
}

bool FilePath::Canonicalize (bool quiet)
{
	char canonicalPath [MAX_PATH];
	char const * path = GetDir ();
	if (!::PathCanonicalize (canonicalPath, path))
	{
		if (quiet)
			return false;
		else
			throw Win::Exception ("Illegal path -- cannot canonicalize.", path);
	}
	Change (canonicalPath);
	return true;
}

void FilePath::ConvertToLongPath ()
{
	if (FilePath::IsAbsolute (GetDir ()))
	{
		FullPathSeq pathSeq (GetDir ());
		FilePath workPath (pathSeq.GetHead ());
		// For every path segment get its long name
		for (; !pathSeq.AtEnd (); pathSeq.Advance ())
		{
			WIN32_FIND_DATA findData;
			workPath.DirDown (pathSeq.GetSegment ().c_str ());
			HANDLE handle = ::FindFirstFile (workPath.GetDir (), &findData);
			if (handle != INVALID_HANDLE_VALUE)
			{
				workPath.DirUp ();
				workPath.DirDown (findData.cFileName);
				::FindClose (handle);
			}
		}
		Change (workPath.GetDir ());
	}
}

void FilePath::ExpandEnvironmentStrings ()
{
	if (_buf.find ('%') == std::string::npos)
		return;	// No environment variables in the path

	std::vector<char> expandedPath;
	unsigned length = 256; 
	expandedPath.resize (length + 1);
	unsigned expandedLength = ::ExpandEnvironmentStrings (GetDir (), 
												&expandedPath [0], 
												length);
	if (expandedLength >= length)
	{
		// try again with longer buffer
		expandedPath.resize (expandedLength + 1);
		expandedLength = ::ExpandEnvironmentStrings (GetDir (), &expandedPath [0], expandedLength);
	}
	Assert (expandedLength <= expandedPath.size ());
	Change (&expandedPath [0]);
}

bool FilePath::IsAbsolute (char const * path)
{
	FilePathSeq folderSeq (path);
	if (folderSeq.HasDrive ())
	{
		return (std::strlen (path) == 2 || IsSeparator (path [2]));
	}
	return folderSeq.IsUNC ();
}

bool FilePath::IsNetwork (char const  * path)
{
	FilePathSeq folderSeq (path);
	if (folderSeq.IsUNC ())
	{
		return true;
	}
	else if (folderSeq.HasDrive ())
	{
		// Check if this is network drive
		FullPathSeq fullPathSeq (path);
		char const * drive = fullPathSeq.GetHead ().c_str ();
		int driveType = ::GetDriveType (drive);
		return driveType == DRIVE_REMOTE;
	}
	return false;
}

bool FilePath::HasValidDriveLetter (char const * path)
{
	FilePathSeq folderSeq (path);
	if (folderSeq.HasDrive ())
	{
		unsigned int driveBitMask = ::GetLogicalDrives ();
		if (driveBitMask != 0)
		{
			PathSplitter splitter (path);
			std::string drive (splitter.GetDrive ());
			unsigned int index = ToUpper (drive [0]) - 'A';
			unsigned int driveBit = (1 << index);
			return (driveBitMask & driveBit) != 0;
		}
	}
	return true;
}

bool FilePath::IsFileNameOnly (char const  * path)
{
	FilePathSeq pathSeq (path);
	if (pathSeq.HasDrive ())
		return false;

	std::string filePath (path);
	return filePath.find_first_of (FilePath::_pathSeparators) == std::string::npos;
}

bool FilePath::IsEqualDir (char const * path) const
{
	return IsEqualDir (path, GetDir ());
}

bool FilePath::IsEqualDir (std::string const & dir1, std::string const & dir2)
{
	if (dir1.length () != dir2.length ())
		return false;
	std::string::const_iterator seq1 = dir1.begin ();
	std::string::const_iterator seq2 = dir2.begin ();
	for (; seq1 != dir1.end (); ++seq1, ++seq2)
	{
		if (::ToLower (*seq1) != ::ToLower (*seq2))
		{
			if (!IsSeparator (*seq1) || !IsSeparator (*seq2))
				return false;
		}
	}
	return true;
}

bool FilePath::IsDirLess (std::string const & dir1, std::string const & dir2)
{
	std::string::const_iterator seq1 = dir1.begin ();
	std::string::const_iterator seq2 = dir2.begin ();
	for (; seq1 != dir1.end () && seq2 != dir2.end (); ++seq1, ++seq2)
	{
		if (IsSeparator (*seq1))
		{
			if (IsSeparator (*seq2))
				continue;
			else
				return true; // separator is less than any other char
		}
		if (IsSeparator (*seq2))
			return false;

		char c1 = ::ToLower (*seq1);
		char c2 = ::ToLower (*seq2);
		if (c1 != c2)
			return c1 < c2;
	}
	return seq1 == dir1.end () && seq2 != dir2.end ();
}

bool FilePath::IsDirLess (FilePath const & path) const
{
	char const * b1 = _buf.c_str ();
	int len1 = _prefix;
	char const * b2 = path._buf.c_str ();
	int len2 = path._prefix;
	return std::lexicographical_compare (b1, b1 + len1, b2, b2 + len2, PathCharLess);
}

bool FilePath::HasPrefix (char const * path) const
{
	size_t prefixLen = strlen (path);
	if (prefixLen > _buf.length ())
		return false;
	if (prefixLen == 0)
		return true;
	size_t i;
	for (i = 0; i < prefixLen; i++)
	{
		if (::ToLower (_buf [i]) != ::ToLower (path [i]))
		{
			if (!IsSeparator (_buf [i]) || !IsSeparator (path [i]))
				break;
		}
	}
	if (i < prefixLen)
		return false;
	// Check if match found on complete path parts -- not in the middle of folder name
	if (prefixLen < _buf.length ())
	{
		// "path = c:\foo\"
		// "buf  = c:\foo\bar"
		if (IsSeparator (path [prefixLen - 1]))
			return true;
		if (!IsSeparator (_buf [prefixLen]))
			return false;
	}
	return true;
}

bool FilePath::HasSuffix (char const * path) const
{
	size_t suffixLen = strlen (path);
	if (suffixLen > _buf.length ())
		return false;
	if (suffixLen == 0)
		return true;
	int i;
	for (i = suffixLen - 1; i >= 0; --i)
	{
		int j = _buf.length () - (suffixLen - i);
		if (::ToLower (_buf [j]) != ::ToLower (path [i]))
		{
			if (!IsSeparator (_buf [j]) || !IsSeparator (path [i]))
				break;
		}
	}
	if (i > 0)
		return false;
	// Check if match found on complete path parts -- not in the middle of folder name
	if (suffixLen < _buf.length ())
	{
		// "path = \foo\bar.h"
		// "buf  = c:\foo\bar.h"
		if (IsSeparator (path [0]))
			return true;
		int k = _buf.length () - suffixLen - 1;
		if (!IsSeparator (_buf [k]))
			return false;
	}
	return true;
}

bool FilePath::IsValid (std::string const & filePath)
{
	if (filePath.find_first_of (FilePath::IllegalChars) != std::string::npos)
		return false;
	size_t colonPos = filePath.find_first_of (":");
	if (colonPos != std::string::npos && colonPos != 1)
		return false;
	return true;
}

bool FilePath::IsValidPattern (char const * path)
{
	PathSplitter splitter (path);
	std::string filePath;
	filePath = splitter.GetDrive ();
	filePath += splitter.GetDir ();
	std::string fname;
	fname = splitter.GetFileName ();
	fname += splitter.GetExtension ();
	// File path cannot contain illegal characters
	if (filePath.find_first_of (FilePath::IllegalChars) != std::string::npos)
		return false;
	size_t colonPos = filePath.find_first_of (":");
	if (colonPos != std::string::npos && colonPos != 1)
		return false;
	// If file name contains illegal path characters this can be only '?' or '*'
	unsigned int curPos = fname.find_first_of (FilePath::IllegalChars);
	while (curPos != std::string::npos)
	{
		char c = fname [curPos];
		if (c != '?' && c != '*')
			return false;
		curPos = fname.find_first_of (FilePath::IllegalChars, ++curPos);
	}
	return true;
}

TmpPath::TmpPath ()
{
	char tmpBuf [MAX_PATH + 1];
	::GetTempPath (sizeof (tmpBuf), tmpBuf);
	long len = MAX_PATH;
	std::vector<char> longName (len + 1);
	long actual = ::GetLongPathName (tmpBuf, &longName [0], len);
	if (actual > len)
	{
		longName.resize (actual + 1);
		::GetLongPathName (tmpBuf, &longName [0], len);
	}
	Change (&longName [0]);
}

CurrentFolder::CurrentFolder ()
{
	char tmpBuf [MAX_PATH + 1];
	int len = ::GetCurrentDirectory (sizeof (tmpBuf), tmpBuf);
	if (len > sizeof (tmpBuf) || len == 0)
		throw Win::Exception ("Internal error: Cannot get current directory.");
	Change (tmpBuf);
}

void CurrentFolder::Set (char const * path)
{
	Assert (path != 0);
	if (!::SetCurrentDirectory (path))
		throw Win::Exception ("Internal error: Cannot set current directory.", path);
}

CurrentFolderSwitch::CurrentFolderSwitch (char const * path)
{
	Assert (path != 0);
	if (!::SetCurrentDirectory (path))
		throw Win::Exception ("Internal error: Cannot set current directory.", path);
}

CurrentFolderSwitch::~CurrentFolderSwitch ()
{
	::SetCurrentDirectory (_previousCurrentFolder.GetDir ());
}

ProgramFilesPath::ProgramFilesPath ()
{
	RegKey::System keySystem;
	std::string sysPgmFilesPath = keySystem.GetProgramFilesPath ();
	Change (sysPgmFilesPath.c_str ());
}

SystemFilesPath::SystemFilesPath ()
{
	char buf [MAX_PATH + 1];
	int len = ::GetWindowsDirectory (buf, MAX_PATH);
	if (len > sizeof (buf) || len == 0)
		throw Win::Exception ("Internal error: Cannot get system directory.");
	Change (buf);
}

//
// Path iterators: split path into segments between slashes
//

// Generic FilePathSeq

FilePathSeq::FilePathSeq (char const * path)
{
	Assert (path != 0);
	_path = path;
}

bool FilePathSeq::HasDrive () const
{
	// c:\folder
	if (_path.length () >= 2)
	{
		int firstChar = _path [0];
		int secondChar = _path [1];
		if (firstChar < 0 || firstChar > 127)
			return false;
		return isalpha (firstChar) && secondChar == ':';
	}
	return false;
}

bool FilePathSeq::IsUNC () const
{
	// \\server\share\folder
	return _path.length () > 2 && FilePath::IsSeparator (_path [0]) && FilePath::IsSeparator (_path [1]);
}

bool FilePathSeq::IsDriveOnly () const
{
	// c:\ or c:
	if (_path.length () < 2)
		return false;

	int firstChar = _path [0];
	int secondChar = _path [1];
	if (firstChar < 0 || firstChar > 127)
		return false;
	if (!isalpha (firstChar) || secondChar != ':')
		return false;
	return _path.length () == 2 
		|| _path.length () == 3 && FilePath::IsSeparator (_path [2]);
}

// Partial path iterator. Don't call with full path or path ending with separator

PartialPathSeq::PartialPathSeq (char const * path)
	: FilePathSeq (path),
	  _cur (-1),
	  _start (0)
{
	if (_path.empty ())
	{
		_cur = 1;
		return;
	}

	Assert (!HasDrive ());
	Assert (!IsUNC ());
	Assert (!FilePath::IsSeparator (_path [_path.length () - 1]));

	Advance ();
}

bool PartialPathSeq::AtEnd () const
{
	return _cur > _path.length ();
}

void PartialPathSeq::Advance ()
{
	_cur++;
	if (_cur < _path.length ())
	{
		_start = _cur;
		while (_cur < _path.length () && !FilePath::IsSeparator (_path [_cur]))
			_cur++;
		if (_cur < _path.length())
			_path [_cur] = '\0';
	}
}

char const * PartialPathSeq::GetSegment ()
{
	Assert (!AtEnd ());
	return &_path [_start];
}

bool PartialPathSeq::IsLastSegment ()
{
	Assert (!AtEnd ());
	return _cur == _path.length ();
}

// Full path iterator: only call with full path

FullPathSeq::FullPathSeq (char const * path)
	: FilePathSeq (path),
	  _end (0),
	  _start (0)
{
	if (!HasDrive () && !IsUNC ())
		throw Win::InternalException ("Expected absolute path", path);
	if (HasDrive ())
	{
		_head = _path.substr (0, 2);
		_end = 1;
	}
	else
	{
		// \\server\share[\folder]
		size_t headEnd = FindNextSlash (2);
		headEnd = FindNextSlash (headEnd + 1, true); // quiet
		if (headEnd == std::string::npos)
		{
			headEnd = _path.length ();
		}
		_head = _path.substr (0, headEnd);
		_end = _head.length () - 1;
	}
	Advance ();
}

std::string FullPathSeq::GetServerName () const
{
	Assert (IsUNC ());
	size_t slash3Pos = FindNextSlash (2);
	return _path.substr (2, slash3Pos - 2);
}

size_t FullPathSeq::FindNextSlash (size_t off, bool quiet) const
{
	std::string::size_type pos = _path.find_first_of ('\\', off);
	if (pos == std::string::npos)
	{
		if (!quiet)
			throw Win::Exception ("Illegal path", _path.c_str ());
	}

	return pos;
}

void FullPathSeq::Advance ()
{
	Assert (!AtEnd ());
	size_t const pathLen = _path.length ();
	_start = _end + 2; // skip separator
	if (_start >= pathLen)
	{
		_start = pathLen;
		return;
	}
	std::string::const_iterator sep = std::find_if (
		_path.begin () + _start, _path.end (), std::ptr_fun (FilePath::IsSeparator));
	if (sep == _path.end ())
		_end = pathLen - 1;
	else
		_end = sep - _path.begin () - 1;
}

std::string FullPathSeq::GetSegment () const
{
	Assert (!AtEnd ());
	Assert (_end >= _start);
	Assert (!FilePath::IsSeparator (_path [_start]));
	Assert (!FilePath::IsSeparator (_path [_end]));
	return _path.substr (_start, _end - _start + 1);
}

//
// Path Splitter
//

PathSplitter::PathSplitter (char const * path)
{
	_splitpath (path, _drive, _dir, _fname, _ext);
}

PathSplitter::PathSplitter (std::string const & path)
{
	_splitpath (path.c_str (), _drive, _dir, _fname, _ext);
}

bool PathSplitter::IsValidShare () const
{
	// \\machine\share
	std::string directory (_dir);
	int count = std::count_if (directory.begin (), directory.end (), FilePath::IsSeparator);
	return strlen (_drive) == 0   &&
		   count == 3             &&
		   directory.size () > 3  &&
		   FilePath::IsSeparator (_dir [0]) &&
		   FilePath::IsSeparator (_dir [1]) &&
		   FilePath::IsSeparator (_dir [directory.size () - 1]) &&
		   strlen (_fname) > 0 &&
		   strlen (_ext) == 0;
}

PathMaker::PathMaker (char const * drive, char const * dir, char const * fname, char const * ext)
{
	_makepath (_path, drive, dir, fname, ext);
}

DirectoryListing::DirectoryListing (std::string const & path)
{
	if (!File::Exists (path.c_str ()))
		return;
	if (!File::IsFolder (path.c_str ()))
	{
		InitWithPath (path);
		return;
	}
	_sourcePath.Change (path);
	for (FileSeq seq (_sourcePath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		_names.push_back (seq.GetName ());
	}
}

void DirectoryListing::InitWithPath (std::string const & path)
{
	if (!File::Exists (path.c_str ()))
		return;
	PathSplitter splitter (path);
	std::string sourcePath (splitter.GetDrive ());
	sourcePath += splitter.GetDir ();
	std::string fname (splitter.GetFileName ());
	fname += splitter.GetExtension ();
	_sourcePath.Change (sourcePath);
	_names.push_back (fname);
}

// ===================================================
namespace UnitTest
{
	char const DriveLetter [] = "g:";
	char const ServerName [] = "ComputerName";
	char const ServerShare [] = "\\\\ComputerName\\share";

	// test driver
	void TestSeq (char const * segments [])
	{
		std::string path (DriveLetter);
		std::string unc (ServerShare);

		unsigned int segmentCount = 0;
		while (segments [segmentCount] != 0)
		{
			path += '\\';
			unc  += '\\';
			path += segments [segmentCount];
			unc  += segments [segmentCount];
			++segmentCount;
		};

		FullPathSeq pathSeq (path.c_str ());
		FullPathSeq uncSeq (unc.c_str ());

		Assert (pathSeq.HasDrive ());
		Assert (!pathSeq.IsDriveOnly ());
		Assert (!pathSeq.IsUNC ());
		Assert (!pathSeq.AtEnd ());
		Assert (pathSeq.GetHead () == DriveLetter);

		Assert (uncSeq.IsUNC ());
		Assert (!uncSeq.HasDrive ());
		Assert (!uncSeq.AtEnd ());
		Assert (uncSeq.GetServerName () == ServerName);
		Assert (uncSeq.GetHead () == ServerShare);
	
		for (unsigned int i = 0; i < segmentCount; ++i)
		{
			Assert (!pathSeq.AtEnd ());
			Assert (!uncSeq.AtEnd ());
			Assert (pathSeq.GetSegment () == segments [i]);
			Assert (uncSeq.GetSegment () == segments [i]);
			pathSeq.Advance ();
			uncSeq.Advance ();
		}
		Assert (pathSeq.AtEnd ());
		Assert (uncSeq.AtEnd ());

	}

	void FullPathSeqTest (std::ostream & out)
	{
		out << "Test FullPathSeq class." << std::endl;
		{
			// only drive letter
			FullPathSeq drive ("c:");
			Assert (drive.HasDrive ());
			Assert (drive.IsDriveOnly ());
			Assert (!drive.IsUNC ());
			Assert (drive.AtEnd ());
		}
		{
			// drive letter with slash
			FullPathSeq drive ("c:\\");
			Assert (drive.HasDrive ());
			Assert (drive.IsDriveOnly ());
			Assert (!drive.IsUNC ());
			Assert (drive.AtEnd ());
		}
		{
			FullPathSeq unc (ServerShare);
			Assert (unc.IsUNC ());
			Assert (!unc.HasDrive ());
			Assert (unc.AtEnd ());
			Assert (unc.GetServerName () == ServerName);
			Assert (unc.GetHead () == ServerShare);
		}
		{
			std::string serverShareWithSlash = ServerShare;
			serverShareWithSlash += "\\";
			FullPathSeq unc (ServerShare);
			Assert (unc.IsUNC ());
			Assert (!unc.HasDrive ());
			Assert (unc.AtEnd ());
			Assert (unc.GetServerName () == ServerName);
			Assert (unc.GetHead () == ServerShare);
		}
		{
			char const * segments1 [] = { "a", 0 };
			char const * segments2 [] = { "abcd", 0 };
			char const * segments3 [] = { "a", "b", "c", "d", "e", 0 };
			char const * segments4 [] = { "aa", "bb", "cc", "dd", "ee", "ff", "gg", "hh", 0 };
			char const * segments5 [] = 
			{
				"s", "se", "seg", "segm", "segme", "segmen", "segment", 0
			};
			char const * segments6 [] = 
			{
				"segment", "segmen", "segme", "segm", "seg", "se", "s", 0
			};

			TestSeq (segments1);
			TestSeq (segments2);
			TestSeq (segments3);
			TestSeq (segments4);
			TestSeq (segments5);
			TestSeq (segments6);
		}
		out << "Passed!" << std::endl;
	}
}
