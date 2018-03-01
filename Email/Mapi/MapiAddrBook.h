#if !defined (MAPIADDRBOOK_H)
#define MAPIADDRBOOK_H
//
// (c) Reliable Software 1998 -- 2001
//

#include "MapiIface.h"

#include <mapix.h>

namespace Mapi
{
	class DefaultDirectory;
	class AddrList;

	class AddressBook
	{
	public:
		AddressBook (DefaultDirectory & defDir);

		bool ResolveName (AddrList & addrList);
		Interface<IAddrBook> & Get () { return _addrBook; }

	private:
		Interface<IAddrBook>	_addrBook;
	};
};

#endif
