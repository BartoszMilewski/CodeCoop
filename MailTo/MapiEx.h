#if !defined (MAPIEX_H)
#define MAPIEX_H
//
// (c) Reliable Software 1998
//

#include <Ex/WinEx.h>

struct IMAPIProp;

class MapiException : public Win::Exception
{
public:
	MapiException (char const * msg, HRESULT hRes, char const * obj = 0);
};

char const * MapiExtendedError (HRESULT hRes, struct IMAPIProp * pIf);

#endif
