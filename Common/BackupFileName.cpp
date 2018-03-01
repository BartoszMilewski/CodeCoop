//----------------------------------
//  (c) Reliable Software, 2008
//----------------------------------

#include "precompiled.h"
#include "BackupFileName.h"
#include "RegFunc.h"

#include <TimeStamp.h>
#include <File/File.h>

BackupFileName::BackupFileName ()
{
	_fileName  = "Code Co-op Backup for ";
	_fileName += Registry::GetComputerName ();
	StrDate timeStamp (CurrentTime ());
	_fileName += " ";
	_fileName += timeStamp.GetString ();
	_fileName += ".cab";
	File::LegalizeName (_fileName);
}
