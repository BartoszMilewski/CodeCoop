// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------

#include "precompiled.h"
#include "RegFunc.h"
#include <Sys/RegKey.h>

std::string Registry::GetComputerName ()
{
	RegKey::Root keyRoot (HKEY_LOCAL_MACHINE);
	RegKey::ReadOnly keyMain (keyRoot, "System");
	RegKey::Check keyControlSet (keyMain, "CurrentControlSet");
	RegKey::Check keyControl (keyControlSet, "Control");
	RegKey::Check keyComputerName (keyControl, "ComputerName");
	RegKey::Check keyActiveComputerName (keyComputerName, "ActiveComputerName");
	if (keyActiveComputerName.Exists ())
	{
		return keyActiveComputerName.GetStringVal ("ComputerName");
	}
	return std::string ();
}
