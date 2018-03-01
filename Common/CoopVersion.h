#if !defined (COOPVERSION_H)
#define COOPVERSION_H
// ----------------------------------
// (c) Reliable Software, 2003 - 2007
// ----------------------------------

#include "Global.h"

class CoopVersion 
{
public:
	CoopVersion (std::string const & version);

	std::string const & GetMajor () const { return _major; }
	std::string const & GetMinor () const { return _minor; }
	std::string const & GetPatch () const { return _patch; }
	std::string const & GetBuild () const { return _build; }

	bool IsEarlier (CoopVersion const & ver) const;
	bool IsMajorEqual (CoopVersion const & ver) const;
	bool IsBeta () const { return !_build.empty (); }

	static bool IsPro ()
	{
		return TheCurrentProductId == coopProId;
	}

	static bool IsLite ()
	{
		return TheCurrentProductId == coopLiteId;
	}

	static bool IsWiki ()
	{
		return TheCurrentProductId == clubWikiId;
	}

private:
	std::string		_major;
	std::string		_minor;
	std::string		_patch;
	std::string		_build;
};

#endif
