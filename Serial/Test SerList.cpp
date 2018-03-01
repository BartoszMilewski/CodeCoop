//
// Reliable Software (c) 1997
//
#if !defined NDEBUG

#include "SerList.h"

// Test

class Item: public DLink, public Serializable
{
public:
	Item (int i = 0) :_i (i) {}
	Item (Item const & item) : _i (item._i){}
	Item (Deserializer& in, int version)
	{
		Deserialize (in, version);
	}
    void Serialize (Serializer& out) const {}
    void Deserialize (Deserializer& in, int version) {}
private:
	int _i;
};

void TestList ()
{
	SerList<Item> list;
	Item item (10);

	list.Add (&item);
	SerList<Item> list2 (list);
	RemoveListIter<Item> itr (list);
	itr.Remove ();
	for (DListIter<Item> it (list2); !it.AtEnd (); it.Advance ())
	{
		Item const * i = it.GetItem ();
	}
}

#endif