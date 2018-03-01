//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include <WinLibBase.h>
#include "NetShare98.h"
#include "ShareInfo98.h"
#include "ShareEx.h"

using namespace Net;

Share98::Share98 ()
	: _dll ("svrapi.dll")
{
	 _dll.GetFunction ("NetShareAdd", _addFunc);
	 _dll.GetFunction ("NetShareDel", _delFunc);
}

void Share98::Add (SharedObject const & obj)
{
	ShareInfo50 info (obj);
	// add netshare
	NET_API_STATUS status = (_addFunc) (
		0,							// server name (0 - local)
		50,							// information level, must be 50 for Win95/98
		(char const *)&info,		// share info
		sizeof (share_info_50));

	if (status != 0 && status != NERR_DuplicateShare)
		throw ShareException (status, "Net share creation failed", obj.GetNetName(), obj.GetPath());
}

void Share98::Delete (std::string const & netname)
{
	// name must be all caps
	std::string name;
	name.resize (netname.size () + 1);
	std::transform (netname.begin (), netname.end (), 
		name.begin (),
		::ToUpper);

	// delete netshare
	NET_API_STATUS status = (_delFunc) (
		0,						// server name (0 - local)
		&name [0],				// share netname
		0);						// reserved	
	
	if (status != 0 && status != NERR_NetNameNotFound && status != NERR_ShareNotFound)
		throw ShareException (status, "Net share deletion failed", name.c_str(), 0);
}


