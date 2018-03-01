//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "Serialize.h"
#include "CoopExceptions.h"

#include <Dbg/Assert.h>
#include <Ex/WinEx.h>

std::string Deserializer::_emptyPath;

int  Serializable::VersionNo () const 
{ 
  return 0; 
}

// Returns true if sectionID found in the file, otherwise returns false
// and current file position remains unchanged
bool Serializable::FindSection (Deserializer& in, unsigned long sectionID)
{
    File::Offset startFilePointer = in.GetPosition ();
    File::Offset curFilePointer = startFilePointer;
    File::Size fileSize = in.GetSize ();

    // Are we at file end ?

	if (startFilePointer == File::Offset::Invalid 
		|| curFilePointer.ToMath () + Serializer::SizeOfLong () >= fileSize.ToMath ())
	{
        return false;
	}

    // Walk file sections

    unsigned long section = 0;
    while ((section = in.GetLong ()) != sectionID)
    {
        // Is there enough data to read ?

        if (curFilePointer.ToMath () + 3 * Serializer::SizeOfLong () >= fileSize.ToMath ())
            throw Win::Exception ("Corrupted file -- attempting to read past end of file");
        unsigned long version = in.GetLong ();
        unsigned long size    = in.GetLong ();
        curFilePointer += size + 3 * Serializer::SizeOfLong ();
        
        // Are we at file end ?

        if (curFilePointer.ToMath () + Serializer::SizeOfLong () >= fileSize.ToMath ())
            break;
		if (in.SetPosition (curFilePointer) == File::Offset::Invalid)
            break;
    }

    if (section == sectionID)
    {
        return true;
    }
    else
    {
        in.SetPosition (startFilePointer);
        return false;
    }
}

void Serializable::CheckVersion (int versionRead, int versionExpected) const
{
    if (versionRead > versionExpected)
    {
        throw InaccessibleProject ("Code Co-op cannot read the project database, because it is of newer version\n"
								   "You have to have the latest version of Code Co-op to visit this project");
    }
    if (!ConversionSupported (versionRead))
    {
		throw InaccessibleProject ("Code Co-op cannot read the project database, because it was created using a very old version\n"
								   "You have to use an older version of Code Co-op to convert this database");
    }
}

// Section ID
// Version No
// Size

// Section ID
// Version No 
// MAX_LOW_SECTION_SIZE
// Size Hi
// Size Lo

const unsigned MAX_LOW_SECTION_SIZE = 0xffffffff;


void EncodeSize(long long fullSize, Serializer & out)
{
	if (fullSize < MAX_LOW_SECTION_SIZE)
	{
		out.PutLong (static_cast<long>(fullSize));
	}
	else
	{
		// Special hack for sections sizes that don't fit into 32-bit longs
		out.PutLong(MAX_LOW_SECTION_SIZE);
		LargeInteger largeSize = fullSize;
		out.PutLong(largeSize.High());
		out.PutLong(largeSize.Low());
	}
}

void Serializable::Save (Serializer& out) const
{
    if (!IsSection ())
    {
	    Serialize (out);
		return;
	}
	
	// It's a section: we need to store its size

    if (out.IsCounting ())
    {
		CountingSerializer & counter = dynamic_cast<CountingSerializer &>(out);

        // Keep counting from upper level
        out.PutLong (SectionId ());
        Assert (VersionNo () != 0);
        out.PutLong (VersionNo ());

		long long startingSize = counter.GetSize();
		Serialize (out);
		long long fullSize = counter.GetSize() - startingSize;

		// Now for the size: one long or three longs?
		EncodeSize(fullSize, out);
    }
    else
    {
        // Count the size of this section and save
        CountingSerializer counter;
        Serialize (counter);
        Assert (SectionId () != 0);

        out.PutLong (SectionId ());
        Assert (VersionNo () != 0);
        out.PutLong (VersionNo ());
		long long fullSize = counter.GetSize();
		EncodeSize(fullSize, out);

		Serialize (out);
    }
}

// Returns section size
long long Serializable::Read (Deserializer& in)
{
	long long sectionSize = 0;
    int version = 0;

    if (IsSection ())
    {
        if (FindSection (in, SectionId ()))
        {
			sectionSize = 3 * Serializer::SizeOfLong ();
			// Section ID already consumed
            // Get version number
            version = in.GetLong ();
            Assert (version != 0);
            // Get size
			unsigned lowSize = in.GetLong ();
			if (lowSize != MAX_LOW_SECTION_SIZE)
			{
				sectionSize += lowSize;
			}
			else
			{
				// Special hack for sections sizes that don't fit into 32-bit longs
				sectionSize += 2 * Serializer::SizeOfLong ();
				long long actualSize = in.GetLong();
				actualSize <<= 32;
				actualSize += in.GetLong();
				sectionSize += actualSize;
			}
			Deserialize (in, version);
        }
    }
	return sectionSize;
}

unsigned char FileDeserializer::GetByte ()
{
    unsigned char b = 0;
	Read (&b, Serializer::SizeOfByte ());
    return b;
}

long FileDeserializer::GetLong ()
{
    long l = 0;
	Read (&l, Serializer::SizeOfLong ());
    return l;
}

bool FileDeserializer::GetBool ()
{
    long l = 0;
	Read (&l, Serializer::SizeOfBool ());
    if (l == inFileTrue)
        return true;
    else if (l == inFileFalse)
        return false;
    else
        throw Win::Exception ("Corrupt file: GetBool: illegal boolean value");
    return false;
}

void FileSerializer::PutBool (bool f)
{
    long l = f ? inFileTrue : inFileFalse;
	Write (&l, SizeOfBool ());
}

void MemorySerializer::PutByte (BYTE b) 
{
	if (IsFreeSpace (Serializer::SizeOfByte ()))
	{
		unsigned char * writePos = GetWritePos ();
		*writePos = b;
		Advance (Serializer::SizeOfByte ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot write to memory buffer.");
	}
}

void MemorySerializer::PutLong (long l) 
{
	if (IsFreeSpace (Serializer::SizeOfLong ()))
	{
		long * pTmp = reinterpret_cast<long *>(GetWritePos ());
		*pTmp = l;
		Advance (Serializer::SizeOfLong ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot write to memory buffer.");
	}
}

void MemorySerializer::PutBool (bool f)
{
	if (IsFreeSpace (Serializer::SizeOfByte ()))
	{
		unsigned char * writePos = GetWritePos ();
		if (f)
			*writePos = 0xf;
		else
			*writePos = 0;
		Advance (Serializer::SizeOfByte ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot write to memory buffer.");
	}
}

void MemorySerializer::PutBytes (char const * pBuf, unsigned long size)
{
	PutBytes (reinterpret_cast<unsigned char const *> (pBuf), size);
}

void MemorySerializer::PutBytes (unsigned char const * pBuf, unsigned long size)
{
	if (IsFreeSpace (size))
	{
		memcpy (GetWritePos (), pBuf, size);
		Advance (size);
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot write to memory buffer.");
	}
}

unsigned char MemoryDeserializer::GetByte ()
{
	unsigned char b;
	if (IsFreeSpace (Serializer::SizeOfByte ()))
	{
		unsigned char const * readPos = GetReadPos ();
		b = *readPos;
		Advance (Serializer::SizeOfByte ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot read from memory buffer.");
	}
	return b;
}

long MemoryDeserializer::GetLong () 
{
	long l;
	if (IsFreeSpace (Serializer::SizeOfLong ()))
	{
		long const * pTmp = reinterpret_cast<long const *>(GetReadPos ());
		l = *pTmp;
		Advance (Serializer::SizeOfLong ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot read from memory buffer.");
	}
	return l;
}

bool MemoryDeserializer::GetBool ()
{
	bool f;
	if (IsFreeSpace (Serializer::SizeOfByte ()))
	{
		unsigned char const * readPos = GetReadPos ();
		if (*readPos == 0xf)
			f = true;
		else
			f = false;
		Advance (Serializer::SizeOfByte ());
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot read from memory buffer.");
	}
	return f;
}

void MemoryDeserializer::GetBytes (char * pBuf, unsigned long size)
{
	GetBytes (reinterpret_cast<unsigned char *> (pBuf), size);
}

void MemoryDeserializer::GetBytes (unsigned char * pBuf, unsigned long size)
{
	if (IsFreeSpace (size))
	{
		memcpy (pBuf, GetReadPos (), size);
		Advance (size);
	}
	else
	{
		throw Win::Exception ("Internal error: Cannot read from memory buffer.");
	}
}

File::Offset MemoryDeserializer::GetPosition () const
{
	return File::Offset (_begin - _bufStart, 0);
}

File::Offset MemoryDeserializer::SetPosition (File::Offset pos)
{
	if (_size != 0 && (pos.IsLarge () || pos.Low () > _size))
		return File::Offset::Invalid;
	Rewind ();
	Advance (pos.Low ());
	File::Offset currentPos = GetPosition ();
	return currentPos;
}

bool MemoryDeserializer::Rewind ()
{
	_begin = const_cast<unsigned char *>(_bufStart);
	return true;
}
