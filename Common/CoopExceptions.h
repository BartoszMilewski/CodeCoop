#if !defined (COOPEXCEPTIONS_H)
#define COOPEXCEPTIONS_H
//----------------------------------
//  (c) Reliable Software, 2005
//----------------------------------

#include <Ex/WinEx.h>

class InaccessibleProject : public Win::InternalException
{
public:
	InaccessibleProject (char const * msg, char const * objName = 0)
		: Win::InternalException (msg, objName)
	{}
};

#endif
