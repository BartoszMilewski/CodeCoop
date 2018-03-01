//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "Area.h"

namespace Area
{
	char const * Name [] =
	{
		"Project",      // Project Area
		"Original",     // Original Area *.og1 or *.og2
		"Reference",    // Reference Area *.ref
		"Synch",        // Synch Area *.syn
		"PreSynch",		// Project PreSynch Area used during script unpack *.bak
		"Staging",		// Project Staging Area *.prj
		"OriginalBackup",// OriginalBackupArea *.ogx where x is always previous original id
		"Temporary"		// Temporary *.tmp -- reconstructed files
		"Local Edits Copy"
	};
};
