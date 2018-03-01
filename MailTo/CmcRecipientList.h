#if !defined (CMCRECIPIENTLIST_H)
#define CMCRECIPIENTLIST_H
//
// (c) Reliable Software 1998
//

#include "CmcRecipient.h"

#include <windows.h>
#include <xcmc.h>

class AddressBook;

class RecipientList
{
public:
	RecipientList (std::vector<std::string> const & emailAddresses);

	void Verify (AddressBook & addressBook);
	CMC_recipient const * GetCmcAddrList () const { return &_cmcAddrList [0]; }

private:
	auto_vector<CmcRecipient>	_recipients;
	auto_array<CMC_recipient>	_cmcAddrList;
};

#endif
