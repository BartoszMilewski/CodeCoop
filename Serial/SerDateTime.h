#if !defined (SERDATETIME_H)
#define SERDATETIME_H
// (c) Reliable Software, 2004

#include "Serialize.h"
#include <Sys/Date.h>
#include <Sys/PackedTime.h>

class SerDate: public Date, public Serializable
{
public:
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
};

inline void SerDate::Serialize (Serializer& out) const
{
	out.PutLong (_year);
	out.PutLong (_month);
	out.PutLong (_day);
}

inline void SerDate::Deserialize (Deserializer& in, int version)
{
	_year = in.GetLong ();
	_month = in.GetLong ();
	_day = in.GetLong ();
}

class SerPackedTime : public PackedTime, public Serializable
{
public:
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
};

inline void SerPackedTime::Serialize (Serializer& out) const
{
	out.PutLong (_time.dwLowDateTime);
	out.PutLong (_time.dwHighDateTime);
}

inline void SerPackedTime::Deserialize (Deserializer& in, int version)
{
	_time.dwLowDateTime = in.GetLong ();
	_time.dwHighDateTime = in.GetLong ();
}

#endif
