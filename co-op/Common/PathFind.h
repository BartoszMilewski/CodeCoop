#if !defined PATHFINDER_H
#define PATHFINDER_H
//------------------------------------
//  (c) Reliable Software, 1996 - 2007
//------------------------------------

#include "SysPath.h"
#include "Area.h"
#include "ProjectPath.h"
#include "GlobalId.h"

class DataBase;
class UniqueName;

namespace Project
{
	class Data;
}

class PathFinder: public SysPathFinder
{
public:
    PathFinder (DataBase const & dataBase);

	void SetProjectDir (Project::Data const & projData);
    void Clear ();

    char const * GetProjectDir () const { return _baseDir.GetDir (); }
    
    Project::Path const & ProjectDir () const  { return _baseDir; }
    Project::Path const & InBoxDir () const  { return _inBoxDir; }

    void CreateDirs ();
	bool IsFileInProject (GlobalId gid) const;
    char const * GetFullPath (GlobalId fileGid, Area::Location loc);
    char const * GetFullPath (UniqueName const & uname);
    char const * GetRootRelativePath (GlobalId fileGid) const;
	char const * XGetRootRelativePath (GlobalId fileGid) const;
	char const * GetRootRelativePath (UniqueName const & uname) const;
    char const * XGetFullPath (GlobalId fileGid, Area::Location loc);
    char const * XGetFullPath (UniqueName const & uname);
    char const * GetAllFilesPath (Area::Location loc) const;
    char const * XGetAllFilesPath (Area::Location loc) const;

    static void  MaterializeFolderPath (char const * path, bool quiet = false);

private:
    char const * GetFileExtension (Area::Location loc, int orgAreaId) const;
    char const * GetAreaFullPath (GlobalId fileGid, Area::Location loc) const;
    char const * XGetAreaFullPath (GlobalId fileGid, Area::Location loc) const;

private:
    Project::Path			_baseDir;
    Project::XPath			_xBaseDir;
    Project::Path			_inBoxDir;
	mutable Project::Path	_rootRelDir;
	mutable Project::XPath	_xRootRelDir;
    DataBase const &		_dataBase;
};

#endif
