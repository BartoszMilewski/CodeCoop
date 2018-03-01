#if !defined (SIMPLERECIPIENTLIST_H)
#define SIMPLERECIPIENTLIST_H
//
// (c) Reliable Software 1998
//

#include "SimpleRecipient.h"

class AddressBook;

class RecipientList
{
public:
	RecipientList (std::vector<std::string> const & emailAddresses);

	void Verify (AddressBook & addressBook);
	unsigned long GetCount () const { return _recipients.size (); }
	MapiRecipDesc const * GetMapiAddrList () const { return &_addrList [0]; }

private:
	auto_vector<SimpleRecipient>	_recipients;
	auto_array<MapiRecipDesc>		_addrList;
};

#endif
