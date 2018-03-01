#if !defined (SERVECTOR_H)
#define SERVECTOR_H
// (c) Reliable Software, 2004

#include "Serialize.h"
#include <Dbg/Assert.h>

template<class T>
class SerVector: public std::vector<T>, public Serializable
{
public:
	SerVector () {}
	SerVector (std::vector<T> const & vect)
		: std::vector<T> (vect)
	{}

	void Serialize (Serializer& out) const;
	void Deserialize (Deserializer& in, int version);
};

template <class T>
void SerVector<T>::Serialize (Serializer& out) const
{
	out.PutLong (size ());
	for (std::vector<T>::const_iterator it = begin (); it != end (); ++it)
		it->Serialize (out);
}

template <class T>
void SerVector<T>::Deserialize (Deserializer& in, int version)
{
	Assert (size () == 0);
	unsigned int count = in.GetLong ();
	// Assumption: at least one byte per entry must be read from file
	if (count > in.GetSize ().Low ())
		throw Win::Exception ("Deserializing corrupted file");
	for (unsigned int i = 0; i < count; i++)
	{
		push_back (T (in, version));
	}
	Assert (size () == count);
}

template<>
inline void SerVector<unsigned>::Serialize (Serializer& out) const
{
	unsigned count = size ();
	out.PutLong (count);
	for (unsigned i = 0; i != count; ++i)
		out.PutLong (operator[](i));
}

template<>
inline void SerVector<unsigned>::Deserialize (Deserializer& in, int version)
{
	Assert (size () == 0);
	unsigned int count = in.GetLong ();
	for (unsigned int i = 0; i < count; i++)
	{
		long x = in.GetLong ();
		push_back (static_cast<unsigned> (x));
	}
	Assert (size () == count);
}

template<>
inline void SerVector<unsigned char>::Serialize (Serializer& out) const
{
	unsigned count = size ();
	out.PutLong (count);
	out.PutBytes (&(operator [] (0)), count);
}

template<>
inline void SerVector<unsigned char>::Deserialize (Deserializer& in, int version)
{
	Assert (size () == 0);
	unsigned int count = in.GetLong ();
	resize (count);
	in.GetBytes (&(operator [] (0)), count);
}

#endif
