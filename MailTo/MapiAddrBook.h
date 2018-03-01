#if !defined (MAPIADDRBOOK_H)
#define MAPIADDRBOOK_H
//
// (c) Reliable Software 1998
//

#include <Com\Shell.h>

struct IAddrBook;
class Session;

class AddressBook : public SIfacePtr<IAddrBook>
{
public:
	AddressBook (Session & session);
};

#endif
