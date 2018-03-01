#if !defined (SERLIST_H)
#define SERLIST_H
//
// Reliable Software (c) 1997, 98, 99, 2000
//
#include "Serialize.h"
#include <Dbg/Assert.h>

template<class T>
class SerList: public std::list<T>, public Serializable
{
public:
	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
};

template <class T>
void SerList<T>::Serialize (Serializer& out) const
{
	out.PutLong (size ());
	for (std::list<T>::const_iterator it = begin (); it != end (); ++it)
		it->Serialize (out);
}

template <class T>
void SerList<T>::Deserialize (Deserializer& in, int version)
{
	Assert (size () == 0);
	unsigned int count = in.GetLong ();
	// Assumption: at least one byte per entry has to be read from file
	if (count > in.GetSize ().Low ())
		throw Win::Exception ("Deserializing corrupted file");
	for (unsigned int i = 0; i < count; i++)
	{
		push_back (T (in, version));
	}
	Assert (size () == count);
}


#endif
