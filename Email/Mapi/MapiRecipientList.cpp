//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "precompiled.h"
#include "MapiRecipientList.h"
#include "MapiAddrBook.h"

using namespace Mapi;

RecipientList::RecipientList (std::vector<std::string> const & addressVector,
							  AddressBook & addressBook,
							  bool bccRecipients)
	: _addrList (addressVector.size ())
{
	std::vector<std::string>::const_iterator iter = addressVector.begin ();
	if (bccRecipients)
	{
		for (; iter != addressVector.end (); ++iter)
		{
			BccRecipient recipient (*iter);
			_addrList.AddRecipient (recipient);
		}
	}
	else
	{
		for (; iter != addressVector.end (); ++iter)
		{
			ToRecipient recipient (*iter);
			_addrList.AddRecipient (recipient);
		}
	}
	addressBook.ResolveName (_addrList);
}
