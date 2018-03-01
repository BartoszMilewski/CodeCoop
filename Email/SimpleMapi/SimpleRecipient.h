#if !defined (MAILRECIPIENT_H)
#define MAILRECIPIENT_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include <mapi.h>

#include <auto_array.h>

namespace SimpleMapi
{
	class Recipient : public MapiRecipDesc
	{
	public:
		char const * GetDisplayName () const { return lpszName; }

		void AddDisplayName (std::string const & displayName)
		{
			_displayName.assign (displayName);
			lpszName = &_displayName [0];
		}

		void AddAddress (std::string const & addr)
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

	protected:
		Recipient (unsigned long type)
		{
			ulReserved = 0;
			ulRecipClass = type;
			lpszName = 0;
			lpszAddress = 0;
			ulEIDSize = 0;
			lpEntryID = 0;
		}

	private:
		std::string					_displayName;
		std::string					_address;
		auto_array<unsigned char>	_entryId;
	};

	class ToRecipient : public Recipient
	{
	public:
		ToRecipient ()
			: Recipient (MAPI_TO)
		{}
	};

	class CcRecipient : public Recipient
	{
	public:
		CcRecipient ()
			: Recipient (MAPI_CC)
		{}
	};

	class BccRecipient : public Recipient
	{
	public:
		BccRecipient ()
			: Recipient (MAPI_BCC)
		{}
	};
}

#endif
