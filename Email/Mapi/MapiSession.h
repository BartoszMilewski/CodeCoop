#if !defined (MAPISESSION_H)
#define MAPISESSION_H
//
// (c) Reliable Software 1998 -- 2004
//
#include "EmailTransport.h"
#include "MapiHelpers.h"
#include "MapiIface.h"

namespace Mapi
{
	class AddressBook;

	class Use
	{
	public:
		explicit Use (bool isMultithreaded);
		~Use ();

	private:
		static MAPIINIT_0	_mapiInit; 
	};

	class Session: public Email::SessionInterface
	{
	public:
		explicit Session (bool browseOnly = false);
		~Session ();

		void OpenAddressBook (Interface<IAddrBook> & addrBook);
		void GetMsgStoresTable (Interface<IMAPITable> & table);
		void GetStatusTable (Interface<IMAPITable> & table);
		void OpenMsgStore (std::vector<unsigned char> const & id, Interface<IMsgStore> & store);
		Result OpenEntry (EntryId const & id, Com::UnknownPtr & unknown, ObjectType & objType);
		Result QueryIdentity (EntryId & id);

	private:
		Session (Session const &);
		Session & operator= (Session const &);
	private:
		Use						_usesMapi;
		Interface<IMAPISession>	_session;
	};
}

#endif
