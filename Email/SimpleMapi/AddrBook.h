#if !defined (ADDRBOOK_H)
#define ADDRBOOK_H
//----------------------------------
// (c) Reliable Software 1998 - 2005
//----------------------------------

#include "SimpleRecipient.h"

#include <mapi.h>

namespace SimpleMapi
{
	class Session;

	class AddressBook
	{
		friend class ResolvedRecip;
	public:
		AddressBook (Session & session);

		bool VerifyRecipient (std::string & emailAddr) const;

	private:
		MapiRecipDesc * Lookup (char const * recipName) const;
		void Release (MapiRecipDesc * recip) const { _free (recip); }
		typedef ULONG (FAR PASCAL *MapiLookup) (LHANDLE session,
												ULONG ulUIParam,
												LPTSTR lpszName,
												FLAGS flFlags,
												ULONG reserved,
												MapiRecipDesc ** lppRecip);
		typedef ULONG (FAR PASCAL *MapiFree) (LPVOID buf);

		Session &	_session;
		MapiLookup	_lookup;
		MapiFree	_free;
	};

	class ResolvedRecip
	{
	public:
		ResolvedRecip (AddressBook & addrBook, Recipient & recip)
			: _addrBook (addrBook), _recip (0)
		{
			_recip = _addrBook.Lookup (recip.GetDisplayName ());
		}
		~ResolvedRecip ()
		{
			_addrBook.Release (_recip);
		}
		MapiRecipDesc * operator-> () { return _recip; }
	private:
		AddressBook &	_addrBook;
		MapiRecipDesc * _recip;
	};
}

#endif
