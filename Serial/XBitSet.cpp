//-----------------------------------------
//  <FileName>
//  (c) Reliable Software, 2000
//-----------------------------------------

#include "precompiled.h"
#include "XBitSet.h"

void XBitSet::Serialize (Serializer & out) const
{
	out.PutLong (_backup.to_ulong ());
}

void XBitSet::Deserialize (Deserializer & in, int version)
{
	unsigned long long value = in.GetLong ();
	_backup = std::bitset<std::numeric_limits<unsigned long>::digits>(value);
}
