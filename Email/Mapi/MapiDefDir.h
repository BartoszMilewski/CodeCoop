#if !defined (MAPIDEFDIR_H)
#define MAPIDEFDIR_H
//-----------------------------------------------------
//  MapiDefDir.h
//  (c) Reliable Software 2001 -- 2004
//-----------------------------------------------------

#include "MapiSession.h"
#include "MapiStore.h"

namespace Mapi
{
	class AddressBook;
	class MailUser;
	class EntryId;
	class PrimaryIdentityId;

	class DefaultDirectory
	{
	public:
		DefaultDirectory (Session & session, bool browseOnly = false);

		MsgStore & GetMsgStore () { return _msgStore; }
		void GetStatusTable (Interface<IMAPITable> & table);
		void OpenAddressBook (AddressBook & addrBook);
		bool OpenMailUser (EntryId const & id, MailUser & user);
		void QueryIdentity (PrimaryIdentityId & id);
	private:
		DefaultDirectory (DefaultDirectory const &);
		DefaultDirectory & operator= (DefaultDirectory const &);
	private:
		Session	&		_session;
		MsgStoresTable	_storesTable;
		DefaultMsgStore	_msgStore;
	};
}

#endif
