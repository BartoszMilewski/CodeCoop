#if !defined (MAPIEX_H)
#define MAPIEX_H
//
// (c) Reliable Software 1998, 99, 2000, 01
//

#include <Ex/WinEx.h>

namespace SimpleMapi
{
	class Exception : public Win::Exception
	{
	public:
		Exception (char const * msg, HRESULT hRes, char const * obj = 0);
	};
}

#endif
