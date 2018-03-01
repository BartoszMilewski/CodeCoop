#if !defined (SERSTRING_H)
#define SERSTRING_H
//
// (c) Reliable Software 1997
//
#include "Serialize.h"

typedef std::string BaseString;

class SerString : public BaseString, public Serializable
{
public:
    SerString ()
    {}
	SerString (std::string const & str)
		: BaseString (str)
	{}
	SerString (SerString && str)
		: BaseString (std::move(str))
	{}
    SerString (Deserializer& in, int version)
    {
        Deserialize (in, version);
    }
	void Pad (unsigned fixedSize); // make fixed lentgh
	void Trim (); // remove trailing nulls
    void Serialize (Serializer& out) const;
    void Deserialize (Deserializer& in, int version);
};

#endif
