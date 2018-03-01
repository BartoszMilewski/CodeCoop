//----------------------------------
// (c) Reliable Software 2001 - 2007
//----------------------------------

#include "precompiled.h"
#include "CheckSum.h"
#include "Crc.h"

#include <File/LongSequence.h>
#include <File/MemFile.h>
#include <Ex/WinEx.h>

CheckSum::CheckSum (MemFile const & file)
    : _sum (0),
	  _crc (0)
{
	if (file.GetBuf () == 0)
		return;
	if (file.GetSize ().IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
	PutBuf (file.GetBuf (), file.GetSize ().Low ());
}

CheckSum::CheckSum (char const * buffer, File::Size size)
    : _sum (0),
	  _crc (0)
{
	if (size.IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
	PutBuf (buffer, size.Low ());
}

CheckSum::CheckSum (std::string const & filePath)
    : _sum (0),
	  _crc (0)
{
	if (!File::Exists (filePath))
		return;

	Crc::Calc crc;
	for (FileViewRoSeq fileView (filePath); !fileView.AtEnd (); fileView.Advance ())
	{
		unsigned char const * buf = reinterpret_cast<unsigned char const *> (fileView.GetBuf ());
		unsigned size = fileView.GetBufSize ();
		for (unsigned i = 0; i < size; ++i)
		{
			// signed checksum
			_sum += static_cast<char> (buf [i]);
			crc.PutByte (buf [i]);
		}
	}
	_crc = crc.Done ();
}


void CheckSum::PutBuf (char const * buffer, unsigned long size)
{
	unsigned char const * buf = reinterpret_cast<unsigned char const *> (buffer);
	Crc::Calc crc;
    for (unsigned long i = 0; i < size; ++i)
	{
		// signed checksum
        _sum += static_cast<char> (buf [i]);
		crc.PutByte (buf [i]);
	}
	_crc = crc.Done ();
}
