//---------------------------
// (c) Reliable Software 2000
//---------------------------
#include <WinLibBase.h>
#include "NetShare.h"
#include "NetShareNT.h"
#include "NetShare98.h"
#include "ShareEx.h"
#include <Sys/SysVer.h>

using namespace Net;

Share::Share ()
{
	SystemVersion ver;
	if (!ver.IsOK ())
		throw Win::Exception ("Get system version failed");

	// create implementation depending on the operating system
	if (ver.IsWin32Windows ())
		throw Win::Exception("Win98 not supported");
	else if (ver.IsWinNT ())
		_impl.reset (new Net::ShareNT ());
	else
		throw Win::Exception ("NetShare not supported on this platform");
}

void Share::Add (Net::SharedObject const & object) 
{
	try
	{
		_impl->Add (object);
	}
	catch (Net::ShareException & e)
	{
		Win::ClearError ();
		if (e.GetPath() != "")
			throw Win::Exception (e.GetMessage (), e.GetPath());
		else if (e.GetName() != "")
			throw Win::Exception (e.GetMessage (), e.GetName());
		else
			throw Win::Exception (e.GetMessage (), e.GetReason ());
	}
}

void Share::Delete (std::string const & netname)
{
	try
	{
		_impl->Delete (netname);
	}
	catch (Net::ShareException & e)
	{
		Win::ClearError ();
		throw Win::Exception (e.GetMessage (), e.GetReason ());
	}
}

