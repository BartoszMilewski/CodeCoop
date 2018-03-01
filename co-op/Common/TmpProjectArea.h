#if !defined (TMPPROJECTAREA_H)
#define TMPPROJECTAREA_H
//-------------------------------------------
//  TmpProjectArea.h
//  (c) Reliable Software, 1998 -- 2002
//-------------------------------------------

#include "GlobalId.h"
#include "Area.h"

class PathFinder;

class TmpProjectArea
{
public:
	TmpProjectArea (Area::Location areaId = Area::Temporary)
		: _versionTimeStamp (0),
		  _areaId (areaId)
	{}

	void MarkNotReconstructed (GlobalId gid) { _files.erase (gid); }
	void RememberVersion (std::string const & comment, long timeStamp);
	void SetAreaId (Area::Location loc) { _areaId = loc; }
	void FileMove (GlobalId gid, Area::Location areaFrom, PathFinder & pathFinder);
	void FileCopy (GlobalId gid, Area::Location areaFrom, PathFinder & pathFinder);
	void CreateEmptyFile (GlobalId gid, PathFinder & pathFinder);

	bool IsEmpty () const { return _files.empty (); }
	bool IsReconstructed (GlobalId gid) const;

	std::string const & GetVersionComment () const { return _versionComment; }
	long GetVersionTimeStamp () const { return _versionTimeStamp; }
	Area::Location GetAreaId () const { return _areaId; }

	void Cleanup (PathFinder & pathFinder);

private:
	GidSet			_files;
	std::string		_versionComment;
	long			_versionTimeStamp;
	Area::Location	_areaId;
};

#endif
