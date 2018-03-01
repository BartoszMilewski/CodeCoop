#if !defined (CMCEX_H)
#define CMCEX_H
//
// (c) Reliable Software 1998
//

#include <Ex\WinEx.h>

#include <xcmc.h>

class CmcException : public WinException
{
public:
	CmcException (char const * msg, CMC_return_code cmcCode);
};

#endif
