#if !defined (FILETAG_H)
#define FILETAG_H
//---------------------------------------
//  FileTag.h
//  (c) Reliable Software, 2001
//---------------------------------------

#include "GlobalId.h"

class FileTag
{
public:
	FileTag (GlobalId gid)
		: _gid (gid)
	{}
	FileTag (char const * path)
		: _gid (gidInvalid),
		  _path (path)
	{}
	FileTag (std::string const & path)
		: _gid (gidInvalid),
		  _path (path)
	{}
	FileTag (GlobalId gid, char const * path)
		: _gid (gid),
		  _path (path)
	{}
	FileTag (GlobalId gid, std::string const & path)
		: _gid (gid),
		  _path (path)
	{}

	GlobalId Gid () const { return _gid; }
	std::string const & Path () const { return _path; }

private:
	GlobalId	_gid;
	std::string _path;
};

#endif
