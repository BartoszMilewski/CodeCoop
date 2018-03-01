// ----------------------------------
// (c) Reliable Software, 2004 - 2006
// ----------------------------------
#include "precompiled.h"
#include "ScriptName.h"
#include <File/File.h>
#include <iomanip>
#include <sstream>

// Script file name:
// "xxxxxxxx [n-of-m] to <user name> in <project name>.cnk"
// "xxxxxxxx to <user name> in <project name>.snc"

ScriptFileName::ScriptFileName (GlobalId scriptId,
								std::string const & to,
								std::string const & project)
{
	std::ostringstream buf;
	buf << std::setw (8) << std::setfill ('0') << std::hex << scriptId;
	_prefix = buf.str ();
	_suffix = " to ";
	_suffix += to;
	_suffix += " in ";
	_suffix += project;
	File::LegalizeName (_suffix);
}

std::string ScriptFileName::Get (unsigned num, unsigned count)
{
	Assert (count == 1 && num == 1 || count > 1 && num <= count);
	std::ostringstream buf;
	buf << _prefix;
	if (count != 1)
	{
		buf << " [" << num << "-of-" << count << ']';
	}
	buf << _suffix;
	if (buf.tellp () > MAX_SCRIPT_FILE_NAME)
	{
		buf.seekp (MAX_SCRIPT_FILE_NAME);
	}
	if (count != 1)
		buf << ".cnk";
	else
		buf << ".snc";
	buf << std::ends;
	return buf.str ();
}

bool ScriptFileName::AreEqualModuloChunkNumber (std::string const & s1, std::string const & s2)
{
	if (s1.length () < 17 || s2.length () < 17) // can't be chunks
		return s1 == s2;

	if (s1.compare (0, 10, s2, 0, 10) != 0)
		return false;

	unsigned int s1StartPos = 10;
	unsigned int s2StartPos = 10;
	if (s1 [9] == '[')
	{
		// chunks
		std::string::size_type s1DashPos = s1.find ("-of-", 11);
		std::string::size_type s2DashPos = s2.find ("-of-", 11);
		if (s1DashPos != std::string::npos && s2DashPos != std::string::npos)
		{
			s1StartPos = s1DashPos + 1;
			s2StartPos = s2DashPos + 1;
		}
	}

	return s1.compare (s1StartPos, s1.length () - s1StartPos, 
					   s2, s2StartPos, s2.length () - s2StartPos) == 0;
}
