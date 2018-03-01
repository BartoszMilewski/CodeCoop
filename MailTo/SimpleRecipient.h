#if !defined (SIMPLERECIPIENT_H)
#define SIMPLERECIPIENT_H
//
// (c) Reliable Software 1998
//

#include "LightString.h"
#include "StrongArray.h"

#include <windows.h>
#include <mapi.h>

class SimpleRecipient : public MapiRecipDesc
{
public:
	SimpleRecipient ()
	{
		ulReserved = 0;
		ulRecipClass = MAPI_TO;
		lpszName = 0;
		lpszAddress = 0;
		ulEIDSize = 0;
		lpEntryID = 0;
	}

	char const * GetDisplayName () const { return lpszName; }

	void AddDisplayName (char const * displayName)
	{
		_displayName.assign (displayName);
		lpszName = const_cast<char *>(_displayName.c_str ());
	}

	void AddAddress (char const * addr)
	{
		_address.assign (addr);
		lpszAddress = const_cast<char *>(_address.c_str ());
	}

	void AddEntryId (unsigned long size, auto_array<unsigned char> & id)
	{
		ulEIDSize = size;
		_entryId = id;
		lpEntryID = &_entryId [0];
	}

private:
	std::string	_displayName;
	std::string	_address;
	auto_array<unsigned char> _entryId;
};

#endif
