#if !defined (MAPISESSION_H)
#define MAPISESSION_H
//
// (c) Reliable Software 1998
//

#include <Com\Shell.h>

struct IMAPISession;

class Session : public SIfacePtr<IMAPISession>
{
public:
	Session ();
	~Session ();
};

#endif
