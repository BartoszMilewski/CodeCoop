#if !defined (MAPIRECIPIENTLIST_H)
#define MAPIRECIPIENTLIST_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "MapiAddrList.h"

namespace Mapi
{
	class AddressBook;

	class RecipientList
	{
	public:
		RecipientList (std::vector<std::string> const & addressVector,
					   AddressBook & addressBook,
					   bool bccRecipients);

		AddrList const & GetAddrList () const { return _addrList; }

	private:
		AddrList	_addrList;
	};
};

#endif
