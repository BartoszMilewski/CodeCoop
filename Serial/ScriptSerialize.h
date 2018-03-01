#if !defined SCRIPT_SERIALIZE_H
#define SCRIPT_SERIALIZE_H
//---------------------------------------------------
//	(c) Reliable Software, 2001 -- 2003
//---------------------------------------------------

#include "Serialize.h"

// Scripts have a more complex versioning system
// We must be able to deserialize very old scripts
// in history and sometimes scripts with future version numbers
// (for transition compatibility)

class ScriptSerializable: public Serializable
{
protected:
	bool ConversionSupported (int versionRead) const { return true; }
	void CheckVersion (int versionRead, int versionExpected) const;
};

#endif
