#if !defined (SCRIPTNAME_H)
#define SCRIPTNAME_H
// (c) Reliable Software, 2004
#include "GlobalId.h"

class ScriptFileName
{
public:
	static bool AreEqualModuloChunkNumber (std::string const & s1, std::string const & s2);
public:
	ScriptFileName (GlobalId scriptId,
					std::string const & to, 
					std::string const & project);
	std::string Get (unsigned num = 1, unsigned count = 1);
private:
	enum
	{
		MAX_SCRIPT_FILE_NAME = 124	// Without file extension
	};

	std::string _prefix;
	std::string _suffix;
};

#endif
