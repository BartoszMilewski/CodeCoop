#if !defined (NETSHARE_NT_H)
#define NETSHARE_NT_H
//---------------------------
// (c) Reliable Software 2000-04
//---------------------------
#include "NetShareImpl.h"
#include <Sys/Dll.h>
#include <lmcons.h>

namespace Net
{
	class ShareNT: public ShareImpl
	{
		typedef NET_API_STATUS (__stdcall *ShareAdd) (LPWSTR servername, 
														 DWORD level, 
														 LPBYTE buf, 
														 LPDWORD parm_err);

		typedef	NET_API_STATUS (__stdcall *ShareDel) (LPWSTR servername, 
														 LPWSTR netname, 
														 DWORD reserved);

	public:
		ShareNT ();

		void Add (SharedObject const & object);
		void Delete (std::string const & netname);

	private:
		Dll			_dll;
		ShareAdd	_pNetShareAdd;
		ShareDel	_pNetShareDel;
	};
}
#endif
