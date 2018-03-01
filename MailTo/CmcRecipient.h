#if !defined (CMCRECIPIENT_H)
#define CMCRECIPIENT_H
//
// (c) Reliable Software 1998
//

#include "LightString.h"

#include <windows.h>
#include <xcmc.h>

class CmcRecipient : public CMC_recipient
{
public:
	CmcRecipient ()
	{
		name = 0;
		name_type = CMC_TYPE_INDIVIDUAL;
		address = 0;
		role = CMC_ROLE_TO;
		recip_flags = 0;
		recip_extensions = 0;
	}

	void AddDisplayName (char const * displayName)
	{
		_displayName.assign (displayName);
		name = const_cast<char *>(_displayName.c_str ());
	}

	void AddAddress (char const * addr)
	{
		_address.assign (addr);
		address = const_cast<char *>(_address.c_str ());
	}

private:
	std::string	_displayName;
	std::string	_address;
};

#endif
