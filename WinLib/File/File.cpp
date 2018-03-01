//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "File.h"
#include "MemFile.h"
#include "ErrorLog.h"
#include "Path.h"
#include "Dir.h"
#include <Sys/Security.h>
#include <StringOp.h>
#include <Ex/Error.h>

#include <memory.h>

char const * File::Vpath::SLASHES = "\\/";

bool File::InitHandle (HANDLE hFile)
{
	_hFile = hFile;
	if (_hFile != INVALID_HANDLE_VALUE)
	{
		InitSizer ();
		return true;
	}
	else
	{
		_sizer.reset ();
		return false;
	}
}

// Since we never use negative offsets, -1 is invalid
File::Offset File::Offset::Invalid (-1, -1);

char const File::NameIllegalChars [] = "<>:\"/\\|\n\r\t*?";
CharSet const File::NameIllegalCharSet (File::NameIllegalChars);

bool File::IsValidName (char const * name)  throw ()
{
	std::string fileName (name);
	return fileName.find_first_of (NameIllegalChars) == std::string::npos;
}

bool File::IsValidExtension (char const * extension)  throw ()
{
	std::string ext (extension);
	if (ext.find_first_of (NameIllegalChars) == std::string::npos)
	{
		return ext.find_first_of ('.') == std::string::npos;
	}
	return false;
}

void File::LegalizeName (std::string & name, char replacementChar) throw ()
{
	std::transform (name.begin (), name.end (), name.begin (),
					File::LegalizeChar (File::NameIllegalCharSet, replacementChar));
}

// Early exit when timestamps differ
bool File::IsEqualQuick (char const * pathOld, char const * pathNew)
{
    if (!File::Exists (pathOld) || !File::Exists (pathNew))
        return false;	// One of the files desn't exist -- can't be equal


	// Quick test
	FileInfo oldInfo (pathOld);
	FileInfo newInfo (pathNew);
	FILETIME oldTime;
	FILETIME newTime;
	oldInfo.GetWriteTime (oldTime);
	newInfo.GetWriteTime (newTime);

	if (CompareFileTime (&newTime, &oldTime) != 0L)
		return false;	// File times are different -- can't be equal

	// Times are equal, try sizes
	if (newInfo.GetSize () != oldInfo.GetSize ())
		return false;	// File size are different -- can't be equal

	if (oldInfo.GetSize ().IsLarge ())
		return false; // cheating for files > 4GB

	// Same size and time, try contents
    MemFileReadOnly oldMem (oldInfo);
    MemFileReadOnly newMem (newInfo);
    return memcmp (newMem.GetBuf (), oldMem.GetBuf (), oldMem.GetSize ().Low ()) == 0;
}

bool File::IsContentsEqual (char const * pathOld, char const * pathNew)
{
    if (!File::Exists (pathOld) || !File::Exists (pathNew))
        return false;	// One of the files desn't exist -- can't be equal
 
	FileInfo oldInfo (pathOld);
	FileInfo newInfo (pathNew);
    // Quick size comparison
	if (newInfo.GetSize () != oldInfo.GetSize ())
		return false;	// File size are different -- can't be equal

	if (oldInfo.GetSize ().IsLarge ())
		return false; // cheating for files > 4GB

    MemFileReadOnly oldMem (oldInfo);
    MemFileReadOnly newMem (newInfo);
    return memcmp (newMem.GetBuf (), oldMem.GetBuf (), oldMem.GetSize ().Low ()) == 0;
}

File::Attributes::Attributes (char const * filePath)
{
	Init (filePath);
}

File::Attributes::Attributes (std::string const & filePath)
{
    Init (filePath.c_str ());
}

void File::Attributes::Init (char const * path)
{
    _bits = ::GetFileAttributes (path);
	if (_bits == INVALID_FILE_ATTRIBUTES)
	{
		LastSysErr sysErr;
		if (sysErr.IsFileNotFound () || sysErr.IsPathNotFound ())
		{
			// Path doesn't exists -- clear file not found error
			Win::ClearError ();
		}
		// else report system error
	}
}

bool File::Exists (std::string const & path)  throw ()
{
	File::Attributes attrs (path);
	return attrs.AreValid ();
}

bool File::IsFolder (char const * path)  throw ()
{
	File::Attributes attrs (path);
	return attrs.AreValid () && attrs.IsFolder ();
}

bool File::IsTreeEmpty (char const * root)
{
	FilePath folder (root);
	for (FileSeq seq (folder.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.IsFolder ())
		{
			folder.DirDown (seq.GetName ());
			if (!IsTreeEmpty (folder.GetDir ()))
				return false;
			folder.DirUp ();
		}
		else
		{
			// We have found a file -- tree is not empty
			return false;
		}
	}
	return true;
}

// Returns true if 'root' is an empty folder
bool File::CleanupTree (char const * root)
{
	FilePath folder (root);
	bool folderIsEmpty = true;
	for (FileSeq seq (folder.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.IsFolder ())
		{
			folder.DirDown (seq.GetName ());
			if (CleanupTree (folder.GetDir ()))
			{
				// If empty subfolder cannot be removed, this folder is not empty
				bool subFolderIsRemoved = ::RemoveDirectory (folder.GetDir ()) != 0;
				if (folderIsEmpty && !subFolderIsRemoved)
					folderIsEmpty = false;
			}
			else
			{
				folderIsEmpty = false;
			}
			folder.DirUp ();
		}
		else
		{
			// We have found a file -- tree is not empty, cannot cleanup
			folderIsEmpty = false;
		}
	}
	return folderIsEmpty;
}

void File::DeleteTree (std::string const & root)
{
	FilePath folder (root);
	for (FileSeq seq (folder.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.IsFolder ())
		{
			folder.DirDown (seq.GetName ());
			DeleteTree (folder.GetDir ());
			folder.DirUp ();
		}
		else
		{
			File::Delete (folder.GetFilePath (seq.GetName ()));
		}
	}
	RemoveEmptyFolder (folder.GetDir ());
}

unsigned File::CountMissingFolders (char const * path)
{
	FullPathSeq checkSeq (path);
    FilePath workPath (checkSeq.GetHead ());
    unsigned missingFolderCount = 0;
    for (; !checkSeq.AtEnd (); checkSeq.Advance ())
    {
        workPath.DirDown (checkSeq.GetSegment ().c_str ());
        if (!File::Exists (workPath.GetDir ()))
            missingFolderCount++;
    }
	return missingFolderCount;
}

bool File::MaterializePath (char const * path, bool quiet)
{
	FullPathSeq folderSeq (path);
    FilePath workPath (folderSeq.GetHead ());
	for (; !folderSeq.AtEnd (); folderSeq.Advance ())
	{
		workPath.DirDown (folderSeq.GetSegment ().c_str ());
		char const * path = workPath.GetDir ();
		if (File::Exists (path))
			continue;	// Skip existing folders
		// If not quiet - throw exception if cannot create folder
		if (!File::CreateFolder (path, quiet))
			return false;
	}
	return true;
}

bool File::CreateFolder (char const * path, bool quiet)
{
	if (!Exists (path))
	{
		if (!::CreateDirectory (path, 0))
		{
			if (quiet)
			{
				Win::ClearError ();
				return false;
			}
			else
			{
				throw Win::Exception ("Cannot create folder.", path);
			}
		}
	}
	return true;
}

bool File::CreateNewFolder (char const * path, bool quiet)
{
	if (!Exists (path))
	{
		if (!::CreateDirectory (path, 0))
		{
			if (quiet)
			{
				Win::ClearError ();
				return false;
			}
			else
			{
				throw Win::Exception ("Cannot create folder.", path);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool File::RemoveEmptyFolder (char const * path, bool quiet)
{
	if (Exists (path))
	{
		if (!::RemoveDirectory (path))
		{
			if (quiet)
			{
				Win::ClearError ();
				return false;
			}
			else
			{
				throw Win::Exception ("Cannot remove folder.", path);
			}
		}
	}
	return true;
}

bool File::IsReadOnly (std::string const & path)  throw ()
{
	File::Attributes attrs (path);
	return attrs.IsReadOnly ();
}

void File::Delete (std::string const & path)
{
    if (Exists (path))
    {
        MakeReadWrite (path);
        if (!::DeleteFile (path.c_str ()))
            throw Win::Exception ("Cannot delete file.", path.c_str ());
    }
}

bool File::DeleteNoEx (std::string const & path) throw ()
{
    if (Exists (path))
    {
		MakeReadWriteNoEx (path);
        // Return false if delete fails
		bool success = ::DeleteFile (path.c_str ()) != 0;
		Win::ClearError ();
		return success;
    }
	return true;
}

bool File::Copy (char const * pathFrom, char const * pathTo)
{
	if (Exists (pathTo))
	{
		MakeReadWrite (pathTo);
	}
	if (!::CopyFile (pathFrom, pathTo, false))
	{
		std::string info ("from: ");
		info += pathFrom;
		info += "\nto: ";
		info += pathTo;
		throw Win::Exception ("Cannot copy file ", info.c_str ());
	}
	return true;
}

bool File::CopyNoEx (char const * pathFrom, char const * pathTo) throw ()
{
	if (Exists (pathTo))
	{
		MakeReadWriteNoEx (pathTo);
	}
	if (!::CopyFile (pathFrom, pathTo, false))
	{
		return false;
	}
	
	return true;
}

bool File::SafeCopy (char const * pathFrom, char const * pathTo)
{
	if (Exists (pathTo))
	{
		Rename2Previous (pathTo);
	}
	return Copy (pathFrom, pathTo);
}

bool File::SafeCopyNoEx (char const * pathFrom, char const * pathTo) throw ()
{
	if (Exists (pathTo))
	{
		Rename2PreviousNoEx (pathTo);
	}
	return CopyNoEx (pathFrom, pathTo);
}

std::string File::Rename2Previous (char const * path)
{
	return File::RenameUnique (path, "previous");
}

std::string File::Rename2PreviousNoEx (char const * path) throw ()
{
	return File::RenameUniqueNoEx (path, "previous");
}

std::string File::Rename2CopyOf (char const * path)
{
	return File::RenameUnique (path, "copy");
}

std::string File::Rename2CopyOfNoEx (char const * path) throw ()
{
	return File::RenameUniqueNoEx (path, "copy");
}

std::string File::CreateUniqueName (char const * path,
									std::string const & additionalNamePart)
{
	if (!File::Exists (path))
		return std::string (path);

	PathSplitter splitter (path);
	std::string pathOnly (splitter.GetDrive ());
	pathOnly += splitter.GetDir ();
	FilePath uniquePath (pathOnly);
	std::string fileNameTemplate (splitter.GetFileName ());
	int count = 1;
	if (additionalNamePart.empty ())
	{
		fileNameTemplate += " (";
		count = 2;
	}
	else
	{
		fileNameTemplate += " (" + additionalNamePart;
	}

	for (; count >= 1; count++)
	{
		std::string uniqueName (fileNameTemplate);
		if (count > 1)
			uniqueName += ' ' + ToString (count);
		uniqueName += ')';
		uniqueName += splitter.GetExtension ();
		char const * newPath = uniquePath.GetFilePath (uniqueName);
		if (!File::Exists (newPath))
			return std::string (newPath);
	}
	throw Win::Exception ("Internal error: cannot create unique file name.", path);
	return std::string ();
}

std::string File::RenameUnique (char const * path, char const * namePrefix)
{
	std::string copyPath;
	if (Exists (path))
	{
		copyPath = CreateUniqueName (path, namePrefix);
		Move (path, copyPath.c_str ());
	}
	return copyPath;
}

std::string File::RenameUniqueNoEx (char const * path, char const * namePrefix) throw ()
{
	std::string copyPath;
	if (Exists (path))
	{
		copyPath = CreateUniqueName (path, namePrefix);
		if (!::MoveFile (path, copyPath.c_str ()))
		{
			return std::string ();	// Empty string -- rename failed
		}
	}
	return copyPath;
}

void File::Move (char const * pathFrom, char const * pathTo)
{
	if (!::MoveFile (pathFrom, pathTo))
	{
		throw Win::Exception ("File error: Cannot move/rename file.", pathFrom);
	}
}

bool File::MoveNoEx (char const * pathFrom, char const * pathTo) throw ()
{
	return ::MoveFile (pathFrom, pathTo) != FALSE;
}

void File::CopyOrDie (char const * pathFrom, char const * pathTo) throw (Win::ExitException)
{
	bool isError = false;
	File::Attributes attrs (pathTo);
	if (attrs.AreValid ())
	{
		attrs.MakeReadWrite ();
		isError = ::SetFileAttributes (pathTo, attrs.ToNative ()) == 0;
	}
	if (!isError)
	{
		isError = ::CopyFile (pathFrom, pathTo, false) == 0;
	}
	if (isError)
	{
		throw Win::ExitException ("Intermittent File Operation Problem\n\nThe program will exit now.\n"
					              "Correct the problem and restart the program to fully recover from this error.",
								  pathTo);
	}
}


void File::Rename2PreviousOrDie (char const * path) throw (Win::ExitException)
{
	if (Exists (path))
	{
		std::string copyPath (CreateUniqueName (path, "previous"));
		if (!::MoveFile (path, copyPath.c_str ()))
		{
			throw Win::ExitException ("Intermittent File Operation Problem\n\nThe program will exit now.\n"
									  "Correct the problem and restart the program to fully recover from this error.",
									  copyPath.c_str ());
		}
	}
}

void File::MakeReadOnly (char const * name)
{
	File::Attributes attrs (name);
	if (attrs.AreValid () && !attrs.IsReadOnly())
	{
		attrs << Attributes::ReadOnly;
        if (::SetFileAttributes (name, attrs.ToNative ()) == 0)
        {
            throw Win::Exception ("File error: Cannot change file attributes to read-only", name);
        }
	}
}

void File::MakeReadWrite (std::string const & name)
{
	File::Attributes attrs (name);
	if (attrs.AreValid () && attrs.IsReadOnly())
	{
		attrs.MakeReadWrite ();
        if (::SetFileAttributes (name.c_str (), attrs.ToNative ()) == 0)
        {
            throw Win::Exception ("File error: Cannot change file attributes to read-write",
									name.c_str ());
        }
	}
}

bool File::MakeReadOnlyNoEx (std::string const & name) throw ()
{
	File::Attributes attrs (name);
	if (attrs.AreValid () && !attrs.IsReadOnly())
	{
		attrs << Attributes::ReadOnly;
		return ::SetFileAttributes (name.c_str (), attrs.ToNative ()) != 0;
	}
	return false;
}

bool File::MakeReadWriteNoEx (std::string const & name) throw ()
{
	File::Attributes attrs (name);
	if (attrs.AreValid () && attrs.IsReadOnly())
	{
		attrs.MakeReadWrite ();
		return ::SetFileAttributes (name.c_str (), attrs.ToNative ()) != 0;
	}
	return false;
}

void File::Touch (char const * path)
{
	FileInfoWritable info (path);
	info.Touch ();
}

bool File::TouchNoEx (std::string const & path) throw ()
{
	File::OpenExistingMode mode;
	File::NormalAttributes attrib;
	HANDLE hFile = ::CreateFile (path.c_str (),
								 mode.GetAccess (),
								 mode.GetShare (),
								 0,
								 mode.GetCreate (),
								 attrib.ToNative (),
								 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	SYSTEMTIME sysTime;
	::GetSystemTime (&sysTime);
	FILETIME fileTime;
	::SystemTimeToFileTime (&sysTime, &fileTime);
	// Revisit: resource management!
    if (!::SetFileTime (hFile, 0, 0, &fileTime))
	{
		::CloseHandle (hFile);
        return false;
	}

	::CloseHandle (hFile);
	return true;
}

bool File::ForceTouchNoEx (std::string const & path) throw ()
{
	if (File::IsReadOnly (path))
	{
		if (!File::MakeReadWriteNoEx (path))
			return false;
		bool success = File::TouchNoEx (path);
		File::MakeReadOnlyNoEx (path);
		return success;
	}
	else
		return File::TouchNoEx (path);
}

//-------------
// File methods
//-------------

File::File (std::string const & path, Mode mode, Attributes attrib)
	: _hFile (INVALID_HANDLE_VALUE)
{
	if (path.empty ())
		return;

	HANDLE hFile = ::CreateFile ( path.c_str (),
							mode.GetAccess (),
							mode.GetShare (),
							0,
							mode.GetCreate (),
							attrib.ToNative (),
							0);
    if (!InitHandle (hFile))
    {
		throw Win::Exception ("File error: Cannot open file or folder.", path.c_str ());
    }
}

File::File (std::string const & path, Mode mode)
	: _hFile (INVALID_HANDLE_VALUE)
{
	if (path.empty ())
	{
		InitHandle (_hFile);
		return;
	}

	Assert (!File::IsFolder (path.c_str ()));
	HANDLE hFile = ::CreateFile ( path.c_str (),
							mode.GetAccess (),
							mode.GetShare (),
							0,
							mode.GetCreate (),
							NormalAttributes ().ToNative (),
							0);
    if (!InitHandle (hFile))
    {
		throw Win::Exception ("File error: Cannot open file or folder.", path.c_str ());
    }
}

bool File::Create (std::string const & path)
{
    HANDLE hFile = ::CreateFile (	path.c_str (),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            0,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            0);
    if (!InitHandle (hFile))
    {
		throw Win::Exception ("File error: Cannot create file.", path.c_str ());
    }
    return true;
}

bool File::Open (std::string const & path, Mode mode, bool quiet)
{
    HANDLE hFile = ::CreateFile ( path.c_str (),
							mode.GetAccess (),
							mode.GetShare (),
							0,
							mode.GetCreate (),
							FILE_ATTRIBUTE_NORMAL,
							0);
    if (!InitHandle (hFile))
    {
		if (!quiet)
			throw Win::Exception ("File error: Cannot open file or folder", path.c_str ());
		else
			throw Win::Exception ();
    }
	Win::ClearError ();	// System may set error like 'cannot create file when it already exists'
	return true;
}

void File::SetAcl (Security::Acl const & acl)
{
	DWORD stat = ::SetSecurityInfo (_hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, acl.Get (), 0);
	if (stat != ERROR_SUCCESS)
		throw Win::Exception ("Cannot set security attributes", "File/Directory", stat);
}

void File::OpenReadOnly (std::string const & path, bool quiet)
{
    HANDLE hFile = ::CreateFile (path.c_str (),
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            0);
    if (!InitHandle (hFile))
    {
		if (!quiet)
			throw Win::Exception ("File error: Cannot open file for reading.", path.c_str ());
		else
			throw Win::Exception ();
    }
}

void File::Close () throw ()
{
    if (FileOk())
    {
        BOOL b = ::CloseHandle (_hFile);
		Assert (b != FALSE);
        InitHandle (INVALID_HANDLE_VALUE);
    }
}

void File::GetWriteTime (FILETIME & lastWrite) const
{
    if (!::GetFileTime (_hFile, 0, 0, &lastWrite))
    {
        throw Win::Exception ("File error: Cannot get file time.");
    }
}

void File::Empty ()
{
	if (!_sizer->Rewind ())
        throw Win::Exception ("Internal error: Cannot empty file.");
    if (!::SetEndOfFile (_hFile))
        throw Win::Exception ("File error: Cannot empty file -- set EOF failure.");
}

File::Size File::GetSize () const
{
	if (_sizer.get ())
	{
		::SetLastError (0);
		return _sizer->GetSize ();
	}
	else
		return File::Size (0, 0);
}

void File::Resize (File::Size newSize)
{
	File::Offset off (newSize.Low (), newSize.High ());
	_sizer->SetPosition (off);

	if (!::SetEndOfFile (_hFile))
		throw Win::Exception ("File resize failed.");
}

FileVersion::FileVersion (std::string const & filePath)
{
	DWORD dummy;
	int size = ::GetFileVersionInfoSize (const_cast<char*>(filePath.c_str ()), &dummy);
	if (size == 0)
		throw Win::Exception ("Cannot read file version info.", 
			                  filePath.c_str ());

	std::vector<char> buf; 
	buf.resize (size);
	if (::GetFileVersionInfo (const_cast<char*>(filePath.c_str ()), 0, size, &buf [0]) == 0)
		throw Win::Exception ("Cannot retrieve file version info.", 
			                  filePath.c_str ());

	void * start;
	unsigned int fixedSize;
	if (::VerQueryValue (&buf [0], "\\", &start, &fixedSize) == 0)
		throw Win::Exception ("This file has no version information.", 
			                  filePath.c_str ());

	Assert (fixedSize <= sizeof (_version));
	::memcpy (&_version, start, fixedSize);
}

void FileInfoWritable::Touch ()
{
	CurrentPackedTime now;
    if (!::SetFileTime (_hFile, 0, 0, now))
        throw Win::Exception ("File error: Cannot change file time.");
}
