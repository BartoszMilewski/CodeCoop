//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include <WinLibBase.h>
#include "MemFile.h"

MappableFile::MappableFile (std::string const & path,
							File::Mode mode)
	: File (path, mode), _hMap (0)
{
	MakeMap (mode.IsReadOnly ());
}

MappableFile::MappableFile (std::string const & path,
						    File::Mode mode,
						    File::Attributes attributes)
	: File (path, mode, attributes), _hMap (0)
{
	MakeMap (mode.IsReadOnly ());
}

MappableFile::MappableFile (std::string const & path,
							File::Mode mode,
							File::Attributes attributes,
							File::Size size)
	: File (path, mode, attributes), _hMap (0)
{
	MakeMap (size, mode.IsReadOnly ());
}

MappableFile::MappableFile (File & file, bool readOnly)
	: File (file), _hMap (0)
{
	MakeMap (readOnly);
}



void MappableFile::Close () throw ()
{
	CloseMap ();
	File::Close ();
}

void MappableFile::MakeMap (File::Size size, bool readOnly)
{
	if (size.IsZero ())
		return;

	_hMap = ::CreateFileMapping ( 
					_hFile,
					0, // security
					readOnly? PAGE_READONLY: PAGE_READWRITE,
					size.High (),
					size.Low (),
					0); // name of the mapping object

	if (_hMap == 0)
	{
		throw Win::Exception ("Create file mapping failed");
	}
}

void MappableFile::MakeMap (bool readOnly)
{
	_hMap = ::CreateFileMapping ( 
					_hFile,
					0, // security
					readOnly? PAGE_READONLY: PAGE_READWRITE,
					0, // map whole file
					0,
					0); // name of the mapping object

	if (_hMap == 0)
	{
		// zero-size files are ok
		File::Size size = GetSize ();
		if (!size.IsZero ())
			throw Win::Exception ("Create file mapping failed");
		Win::ClearError ();
	}
}

void MappableFile::CloseMap () throw ()
{
	if(_hMap != 0)
	{
		::CloseHandle(_hMap);
		_hMap = 0;
	}
}

//---------------------------------------------------

FileViewRo::FileViewRo (std::string const & path)
	: MappableFile (path, File::ReadOnlyMode (), File::SequentialAccessAttributes ()),
	  _viewBuf (0)
{
	SYSTEM_INFO info;
	::GetSystemInfo (&info);
	_pageSize = info.dwAllocationGranularity;
}

FileViewRo::~FileViewRo()
{
	if (_viewBuf != 0)
		::UnmapViewOfFile(_viewBuf);
}

// call with len == 0 to map up to the end of file
char const *  FileViewRo::GetBuf (File::Offset off, unsigned len)
{
	if (_viewBuf != 0 && ::UnmapViewOfFile (_viewBuf) == 0)
		throw Win::Exception ("Cannot release the read-only view of file.");

	long long roundedOffset = off.ToMath();
	roundedOffset /= _pageSize;
	roundedOffset *= _pageSize;

	unsigned inPageOffset = static_cast<unsigned>(off.ToMath() - roundedOffset);

	File::Offset roundedFileOff = roundedOffset;
	unsigned lenToMap = (len == 0)? 0: inPageOffset + len;

	_viewBuf = reinterpret_cast<char const *> (::MapViewOfFile (_hMap,
														  FILE_MAP_READ,
														  roundedFileOff.High (),
														  roundedFileOff.Low (),
														  lenToMap));
	if (_viewBuf == 0)
		throw Win::Exception ("Cannot memory-map file.");

	return _viewBuf + inPageOffset;
}

//---------------------------------------------------

FileViewRoSeq::FileViewRoSeq (std::string const & path)
	: FileViewRo (path),
	  _cur (0, 0),
	  _end (GetSize ().ToMath ()),
	  _viewSize (0)
{
	if (AtEnd ())
		return;		// Zero-size files are ok

	_viewSize = _end.ToMath () < 0x100 * _pageSize ? _end.Low () : 0x100 * _pageSize;
	_viewBuf = reinterpret_cast<char const *> (::MapViewOfFile (_hMap,
														  FILE_MAP_READ,
														  _cur.High (),
														  _cur.Low (),
														  _viewSize));
	if (_viewBuf == 0)
		throw Win::Exception ("Cannot create read-only view of the file.", path.c_str ());
}

void FileViewRoSeq::Advance ()
{
	Assert (!AtEnd ());
	Assert (_viewBuf != 0);
	if (::UnmapViewOfFile (_viewBuf) == 0)
		throw Win::Exception ("Cannot release the read-only view of file.");

	_cur += _viewSize;
	if (AtEnd ())
		return;
	LargeInteger distanceToEnd (_end.ToMath () - _cur.ToMath ());
	Assert (distanceToEnd != 0);
	LargeInteger currentViewSize (_viewSize, 0);
	if (distanceToEnd < currentViewSize)
		_viewSize = distanceToEnd.Low ();
	_viewBuf = reinterpret_cast<char const *> (::MapViewOfFile (_hMap,
														  FILE_MAP_READ,
														  _cur.High (),
														  _cur.Low (),
														  _viewSize));
	if (_viewBuf == 0)
		throw Win::Exception ("Cannot advance read-only view of the file.");
}

//---------------------------------------------------

MemFile::MemFile (std::string const & path, Mode mode, File::Size size)
	: MappableFile (path, mode, File::NormalAttributes (), size),
	  _buf (0),
	  _bufSize (0),
	  _readOnly (mode.IsReadOnly ())
{
	InitBufSize (size);
	Allocate ();
}

MemFile::MemFile (std::string const & path, Mode mode)
	: MappableFile (path, mode),
	  _buf (0),
	  _bufSize (0),
	  _readOnly (mode.IsReadOnly ())
{
	InitBufSize ();
	Allocate ();
}

MemFile::MemFile (File & file, bool readOnly)
	: MappableFile (file, readOnly),
	  _buf (0),
	  _bufSize (0),
	  _readOnly (readOnly)
{
    InitBufSize ();
    Allocate ();
}

MemFile::~MemFile() throw ()
{
	Close ();
}

void MemFile::Close () throw ()
{
	Deallocate ();
	MappableFile::Close ();
}

void MemFile::Flush ()
{
	::FlushViewOfFile (_buf, 0);
}

void MemFile::Deallocate () throw ()
{
	if (_buf != 0)
	{
		::UnmapViewOfFile (_buf);
		_buf = 0;
	}
}

void MemFile::InitBufSize (File::Size size)
{
	if (!size.IsLarge () && size.Low () < MAX_BUFFER)
		_bufSize = size.Low ();
	else
		_bufSize = MAX_BUFFER;
}

void MemFile::InitBufSize ()
{
	File::Size size = GetSize ();
	InitBufSize (size);
}

char * MemFile::ResizeFile (File::Size size)
{
	Assert (!_readOnly);
	Deallocate ();
	CloseMap ();
	MappableFile::Resize (size);
	MakeMap (size, _readOnly);
	InitBufSize (size);
	Allocate ();
	return _buf;
}

char * MemFile::Reallocate (unsigned long bufSize,
							File::Offset fileOffset)
{
	Deallocate ();
	_bufSize = bufSize;
	Allocate (fileOffset);
	return _buf;
}

void MemFile::Allocate (File::Offset fileOffset)
{
	if (_bufSize == 0 || _hMap == 0)
		return;

	_buf = reinterpret_cast<char *> (::MapViewOfFile (
					_hMap,
					_readOnly? FILE_MAP_READ: FILE_MAP_WRITE,
					fileOffset.High (),
					fileOffset.Low (),
					_bufSize));

	if (_buf == 0)
	{
		throw Win::Exception ("Map view of file failed");
	}
}


// Set final size and close
void MemFile::Close (File::Size size)
{
	if (_hFile != INVALID_HANDLE_VALUE)
	{
		Deallocate ();
		CloseMap ();
		File::Resize (size);
		File::Close ();
	}
}

unsigned long MemFile::GetCheckSum () const
{
	if (_buf == 0)
		return 0;
    unsigned long checkSum = 0;
    for (unsigned long i = 0; i < _bufSize; i++)
        checkSum += _buf [i];
    return checkSum;
}

//---------------

// Acquire open file
MemFileReadOnly::MemFileReadOnly (File & file)
    : MemFile (file, true) // read only
{}

//---------------

void LokFile::New (char const *path)
{
    Close ();
	HANDLE hFile = ::CreateFile (
                            path,
                            GENERIC_READ | GENERIC_WRITE,
                            0, // no sharing
                            0,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_HIDDEN,
                            0);
    if (!InitHandle (hFile))
        throw Win::Exception ("Database error: Lock file creation failed.", path);
	MakeMap (File::Size (1, 0), false);
	_bufSize = 1;
    Allocate ();
}

bool LokFile::IsLocked (char const *path) throw ()
{
    HANDLE hFile = ::CreateFile (
                            path,
                            GENERIC_READ | GENERIC_WRITE,
                            0, // no sharing
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_HIDDEN,
                            0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = ::GetLastError ();
		Win::ClearError ();
		if ( err == ERROR_SHARING_VIOLATION)
			return true;
	}
	else
	{
		::CloseHandle (hFile);
	}
	return false;
}

bool LokFile::Open (char const *path)
{
    Close ();
    HANDLE hFile = ::CreateFile (
                            path,
                            GENERIC_READ | GENERIC_WRITE,
                            0, // no sharing
                            0,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_HIDDEN,
                            0);
    if (!InitHandle (hFile))
    {
		unsigned error = ::GetLastError ();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
		{
			Win::ClearError ();
            return false;
		}
        throw Win::Exception ("Database error: Could not open lock file.");
    }
    MakeMap (false);
    InitBufSize ();
    Allocate ();
    return true;
}

void LokFile::Stamp (long timeStamp)
{
    if (!Rewind ())
        throw Win::Exception ("Database error: Cannot rewind lock file");
    unsigned long nBytes = sizeof (timeStamp);
    if (!SimpleWrite (&timeStamp, nBytes) || nBytes != sizeof (timeStamp))
        throw Win::Exception ("Database error: Cannot write to lock file");

    if (!File::Flush ())
        throw Win::Exception ("Database error: Cannot flush lock file");
    if (!Rewind ())
        throw Win::Exception ("Database error: Cannot rewind lock file");
	::UnmapViewOfFile (_buf);
    _buf = 0;
	::CloseHandle(_hMap);
    _hMap = 0;
    MakeMap (false);
    InitBufSize ();
    Allocate ();
}


