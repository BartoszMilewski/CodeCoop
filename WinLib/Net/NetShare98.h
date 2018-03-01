#if !defined (NETSHARE_98_H)
#define NETSHARE_98_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include "NetShareImpl.h"
#include <Sys/Dll.h>
#include <lmcons.h>

namespace Net
{
	class Share98: public ShareImpl
	{
		typedef NET_API_STATUS (__stdcall *ShareAdd) (char const * servername,
														 short level, 
														 char const * buf,
														 unsigned short bufLen);

		typedef	NET_API_STATUS (__stdcall *ShareDel) (char const * server,
														 char const * netname,
														 unsigned short reserved);

	public:
		Share98 ();

		void Add (SharedObject const & object);
		void Delete (std::string const & netname);

	private:
		Dll			_dll;
		ShareAdd	_addFunc;
		ShareDel	_delFunc;
	};
}
#endif
