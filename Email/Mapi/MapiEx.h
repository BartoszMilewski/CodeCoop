#if !defined (MAPIEX_H)
#define MAPIEX_H
//
// (c) Reliable Software 1998 -- 2001
//

#include <Ex/WinEx.h>

namespace Mapi
{
	class Exception : public Win::Exception
	{
	public:
		Exception (char const * msg, HRESULT hRes, char const * obj = 0);
	};
}

#endif
