//
// (c) Reliable Software 1997 -- 2003
//
#include "precompiled.h"
#include "SerString.h"

void SerString::Serialize (Serializer& out) const
{
    size_t len = length ();
    out.PutLong (len);
    if (len != 0)
        out.PutBytes (data (), len);
}

void SerString::Deserialize (Deserializer& in, int version)
{
    size_t len = in.GetLong ();
	if (len > in.GetSize ().Low ())
		throw Win::Exception ("Deserializing corrupted file");
    if (len != 0)
    {
		resize (len);
		char * buf = & operator [] (0);
        in.GetBytes (buf, len);
    }
	else
	{
		clear ();
	}
}

void SerString::Pad (unsigned fixedSize)
{
	resize (fixedSize, '\0');
}

void SerString::Trim ()
{
	resize (strlen (c_str ()));
}

