#if !defined (RECIPIENTLIST_H)
#define RECIPIENTLIST_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "SimpleRecipient.h"
#include <auto_vector.h>

namespace SimpleMapi
{
	class AddressBook;

	class RecipientList
	{
	public:
		RecipientList (std::vector<std::string> const & emailAddresses, bool bccRecipients);

		void Verify (AddressBook & addressBook);
		unsigned long GetCount () const { return _recipients.size (); }
		MapiRecipDesc const * GetMapiAddrList () const { return &_addrList [0]; }

	private:
		auto_vector<Recipient>		_recipients;
		auto_array<MapiRecipDesc>	_addrList;
	};
}

#endif
