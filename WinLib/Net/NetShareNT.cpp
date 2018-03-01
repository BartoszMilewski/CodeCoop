//---------------------------
// (c) Reliable Software 2000-04
//---------------------------
#include <WinLibBase.h>
#include <File/File.h>
#define UNICODE

#include "ShareInfoNT.h"
#include "NetShareNT.h"
#include "ShareEx.h"

using namespace Net;

ShareNT::ShareNT ()
	: _dll ("netapi32.dll")
{		
	_dll.GetFunction<ShareAdd> ("NetShareAdd", _pNetShareAdd);
	_dll.GetFunction<ShareDel> ("NetShareDel", _pNetShareDel);
}

void ShareNT::Add (SharedObject const & obj)
{
	ShareInfo502 shareInfo (obj);
	// if creating a new share
	Security::DescriptorNew secDesc;

	Security::TrusteeEveryone everyone;
	Security::ExplicitAccess accEveryone (everyone, false);

	std::vector<Security::ExplicitAccess> explicitAccList;
	explicitAccList.push_back (accEveryone);

	Security::AccessControlList acl (explicitAccList);
	secDesc.SetAcl (acl);

	shareInfo.SetSecurity (secDesc);
	// add netshare
	NET_API_STATUS status = (_pNetShareAdd) (
		0,						// server name (0 - local)
		shareInfo.Level (),			// information level
		reinterpret_cast<unsigned char*> (&shareInfo),	// share info 
		0);						// wrong param index, not used

	if (status == 0 && File::IsFolder (obj.GetPath ()))
	{
		// Try to give everyone access to the underlying folder
		File::OpenExistingMode mode;
		mode << File::Mode::Access::WriteDac;
		try 
		{
			File dir(obj.GetPath (), mode, File::DirectoryAttributes ());
			Security::Acl oldAcl (dir);
			Security::TrusteeEveryone everyone;
			Security::ExplicitAccess ace (everyone, false);
			std::vector<Security::ExplicitAccess> aces;
			aces.push_back (ace);
			Security::AccessControlList newAcl (aces, oldAcl);

			dir.SetAcl (newAcl);
		}
		catch(...)
		{
			// ignore
		}
	}
	else if (status != NERR_DuplicateShare)
		throw ShareException (status, "Net share creation failed", obj.GetNetName(), obj.GetPath());
}

void ShareNT::Delete (std::string const & netname)
{
	// convert netname to wide string
	std::wstring wname;
	wname.assign (netname.begin (), netname.end ());

	// delete netshare
	NET_API_STATUS status = (_pNetShareDel) (
		0,					// server name (0 - local)
		&wname [0],			// share netname
		0);					// reserved

	if (status != 0 && status != NERR_NetNameNotFound && status != NERR_ShareNotFound)
		throw ShareException (status, "Net share deletion failed", netname.c_str(), 0);
}

