#if !defined (SIMPLEADDRBOOK_H)
#define SIMPLEADDRBOOK_H
//
// (c) Reliable Software 1998
//

#include <windows.h>
#include <mapi.h>

class Session;

class AddressBook
{
public:
	AddressBook (Session & session);

	MapiRecipDesc * Lookup (char const * recipName) const;
	void Release (MapiRecipDesc * recip) const { _free (recip); }

private:
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

#endif
