//-------------------------------------------
//  TmpProjectArea.cpp
//  (c) Reliable Software, 1998 -- 2002
//-------------------------------------------

#include "precompiled.h"
#include "TmpProjectArea.h"
#include "PathFind.h"

#include <File/File.h>

void TmpProjectArea::RememberVersion (std::string const & comment, long timeStamp)
{
	_versionComment = comment;
	_versionTimeStamp = timeStamp;
}

void TmpProjectArea::FileMove (GlobalId gid, Area::Location areaFrom, PathFinder & pathFinder)
{
	std::string from (pathFinder.GetFullPath (gid, areaFrom));
	char const * to = pathFinder.GetFullPath (gid, GetAreaId ());
	File::Move (from.c_str (), to);
	_files.insert (gid);
}

void TmpProjectArea::FileCopy (GlobalId gid, Area::Location areaFrom, PathFinder & pathFinder)
{
	std::string from (pathFinder.GetFullPath (gid, areaFrom));
	char const * to = pathFinder.GetFullPath (gid, GetAreaId ());
	File::Copy (from.c_str (), to);
	_files.insert (gid);
}

void TmpProjectArea::CreateEmptyFile (GlobalId gid, PathFinder & pathFinder)
{
	char const * to = pathFinder.GetFullPath (gid, GetAreaId ());
	File file (to, File::CreateAlwaysMode ());
	_files.insert (gid);
}

bool TmpProjectArea::IsReconstructed (GlobalId gid) const
{
	GidSet::const_iterator iter = _files.find (gid);
	return iter != _files.end ();
}

void TmpProjectArea::Cleanup (PathFinder & pathFinder)
{
	// Delete reconstructed files
	for (GidSet::const_iterator iter = _files.begin (); iter != _files.end (); ++iter)
	{
		char const * path = pathFinder.GetFullPath (*iter, GetAreaId ());
		File::DeleteNoEx (path);
	}
}
