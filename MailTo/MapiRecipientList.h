#if !defined (MAPIRECIPIENTLIST_H)
#define MAPIRECIPIENTLIST_H
//
// (c) Reliable Software 1998
//

#include "MapiRecipient.h"

class AddressBook;

class RecipientList
{
public:
	RecipientList (std::vector<std::string> const & emailAddresses);

	void Verify (AddressBook & addressBook);
	MapiAddrList const & GetMapiAddrList () const { return _mapiAddrList; }

private:
	auto_vector<MapiRecipient>	_recipients;
	MapiAddrList				_mapiAddrList;
};

#endif
