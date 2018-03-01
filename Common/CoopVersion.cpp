// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include "precompiled.h"
#include "CoopVersion.h"

#include <StringOp.h>
#include <Ex/WinEx.h>

// Version string syntax:
// Major + .Minor + Patch   , example: 4.0f
// Major + .Minor + (Build) , example: 4.2 (alpha)
// Major is compared as int
// All other parts are compared as nocase strings
// Minor and Patch are single character strings, Minor is numeric whilst Patch is alphabetic char

CoopVersion::CoopVersion (std::string const & version)
{
	size_t dotPos = version.find ('.');
	if (dotPos == std::string::npos)
	{
		_major = version;
	}
	else
	{
		_major = version.substr (0, dotPos);
		if (version.length () == dotPos + 1)
			throw Win::InternalException ("Unrecognized version number");

		_minor = version.substr (dotPos + 1, 1);
		if (version.length () == dotPos + 2)
			return;
		
		size_t bracketPos = version.find ('(');
		if (bracketPos == std::string::npos)
		{
			_patch = version.substr (dotPos + 2, 1);
		}
		else
		{
			// (Build)
			if (version.length () < bracketPos + 3)
				throw Win::InternalException ("Unrecognized version number");
			_build = version.substr (bracketPos + 1, version.length () - bracketPos - 2);
		}
	}
}

bool CoopVersion::IsEarlier (CoopVersion const & ver) const
{
	if (!IsMajorEqual (ver))
		return ToInt (_major) < ToInt (ver.GetMajor ());

	if (!IsNocaseEqual (_minor, ver.GetMinor ()))
		return IsNocaseLess (_minor, ver.GetMinor ());

	if (IsBeta ())

	{
		if (ver.IsBeta ())
			return IsNocaseLess (_build , ver.GetBuild ());
		else
			return true;
	}

	if (ver.IsBeta ())
		return false;

	return IsNocaseLess (_patch, ver.GetPatch ());
}

bool CoopVersion::IsMajorEqual (CoopVersion const & ver) const
{
	return IsNocaseEqual (_major, ver.GetMajor ());
}
