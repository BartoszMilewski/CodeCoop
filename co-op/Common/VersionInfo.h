#if !defined (VERSIONINFO_H)
#define VERSIONINFO_H
//------------------------------------
//  (c) Reliable Software, 2002 - 2006
//------------------------------------

#include "GlobalId.h"

#include <string>

class VersionInfo
{
public:
	VersionInfo ()
		: _scriptId (gidInvalid),
		  _timeStamp (0)
	{}

	void SetComment (std::string const & comment) { _comment = comment; }
	void SetTimeStamp (long timeStamp) { _timeStamp = timeStamp; }
	void SetVersionId (GlobalId scriptId) { _scriptId = scriptId; }

	GlobalId GetVersionId () const { return _scriptId; }
	std::string const & GetComment () const { return _comment; }
	long GetTimeStamp () const { return _timeStamp; }
	bool IsCurrent () const { return _scriptId == gidInvalid; }

public:
	GlobalId		_scriptId;
	std::string		_comment;
	long			_timeStamp;
};

#endif
