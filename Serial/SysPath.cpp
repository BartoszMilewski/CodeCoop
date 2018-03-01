//----------------------------------
// (c) Reliable Software 1997 - 2005
//----------------------------------

#include "precompiled.h"
#include "SysPath.h"
#include "CoopExceptions.h"

#include <Ex/Winex.h>

char const SysPathFinder::LockFileName []  = "Project.c-o";
char const SysPathFinder::SwitchFileName []  = "switch.bin";

void SysPathFinder::RemoveLock ()
{
    _lokFile.Close ();
    File::DeleteNoEx (GateName ());
}

bool SysPathFinder::LockProject ()
{
	try
	{
		return _lokFile.Open (_dataDir.GetFilePath (LockFileName));
	}
	catch (Win::Exception )
	{
		// Lock file in use -- probably other Code Co-op instance got to it first
		throw InaccessibleProject ("Code Co-op cannot lock project, because it is currently in use.");
	}
}

bool SysPathFinder::IsProjectLocked () throw ()
{
	return LokFile::IsLocked (_dataDir.GetFilePath (LockFileName));
}

void SysPathFinder::UnlockProject () throw ()
{
	_lokFile.Close ();
}

void SysPathFinder::CreateLock ()
{
	_lokFile.New (_dataDir.GetFilePath (LockFileName));
}
