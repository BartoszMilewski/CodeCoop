//-----------------------------------
// (c) Reliable Software 2001 -- 2002
//-----------------------------------

#include "precompiled.h"
#include "ScriptSerialize.h"

#include <Ex/WinEx.h>


void ScriptSerializable::CheckVersion (int versionRead, int versionExpected) const
{
	if (versionRead > versionExpected) 
	{
		throw Win::InternalException ("This script was sent using a newer version of Code Co-op.\n"
									  "You should upgrade your Dispatcher and Code Co-op.");
	}
	if (!ConversionSupported (versionRead))
	{
		throw Win::InternalException ("Cannot read script because it is corrupted.\n"
									  "Ask script sender to re-send it.");
	}

}
