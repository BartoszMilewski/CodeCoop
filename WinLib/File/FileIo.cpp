//----------------------------------
// (c) Reliable Software 1998 - 2006
//----------------------------------

#include <WinLibBase.h>

#include "FileIo.h"

#include <StringOp.h>

File::Offset FileIo::GetPosition () const
{
	return _sizer->GetPosition ();
}

File::Offset FileIo::SetPosition (File::Offset pos)
{
	return _sizer->SetPosition (pos);
}

void FileIo::Read (void * buf, unsigned long cb)
{
    ULONG nBytes = cb;
	if (!SimpleRead (buf, nBytes))
        throw Win::Exception ("Corrupt file: Cannot read from file.");
    // Check for eof 
    if (nBytes != cb)
        throw Win::Exception ("Corrupt file: Attempt to read past end-of-file.");
}

void FileIo::FillBuf (void * buf, unsigned long & cb)
{
    ULONG nBytes = cb;
	if (!SimpleRead (buf, nBytes))
        throw Win::Exception ("Corrupt file: Cannot read from file.");
	cb = nBytes;
}

void FileIo::Write (void const * buf, unsigned long cb)
{
    ULONG nBytes = cb;
    if (!SimpleWrite (buf, nBytes) || nBytes != cb)
        throw Win::Exception ("Internal error: Cannot write to file.");
}

OutStream::OutStream (std::string const & path, std::ios_base::open_mode openMode)
{
	Open (path, openMode);
}

void OutStream::Open (std::string const & path, std::ios_base::open_mode openMode)
{
	Assert (!path.empty ());
	std::wstring wPath (ToWString (path));
	open (wPath.c_str (), openMode);
}

InStream::InStream (std::string const & path, std::ios_base::open_mode openMode)
{
	std::wstring wPath (ToWString (path));
	open (wPath.c_str (), openMode);
}

InOutStream::InOutStream (std::string const & path, std::ios_base::open_mode openMode)
{
	std::wstring wPath (ToWString (path));
	open (wPath.c_str (), openMode);
}