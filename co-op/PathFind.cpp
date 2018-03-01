//----------------------------------
// (c) Reliable Software 1997 - 2009
//----------------------------------

#include "precompiled.h"
#include "PathFind.h"
#include "DataBase.h"
#include "ProjectData.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "Global.h"

#include <Com/Shell.h>
#include <Dbg/Assert.h>
#include <Ex/WinEx.h>

#include <iomanip>

//
// Project Path Finder
//

PathFinder::PathFinder (DataBase const & dataBase)
	: _baseDir (dataBase),
	  _xBaseDir (dataBase),
	  _inBoxDir (dataBase),
	  _rootRelDir (dataBase),
	  _xRootRelDir (dataBase),
	  _dataBase (dataBase)
{}

void PathFinder::SetProjectDir (Project::Data const & projData)
{
    Init (projData.GetDataPath ());
    // c:\My Project
    _baseDir.Change (projData.GetRootDir ());
	_xBaseDir.Change (projData.GetRootDir ());
	Assert (!projData.GetInboxDir ().IsDirStrEmpty ());
    _inBoxDir.Change (projData.GetInboxDir ());
}

void PathFinder::Clear ()
{
    SysPathFinder::Clear ();
    _baseDir.Clear ();
	_xBaseDir.Clear ();
    _inBoxDir.Clear ();
	_rootRelDir.Clear ();
	_xRootRelDir.Clear ();
}

bool PathFinder::IsFileInProject (GlobalId gid) const
{
	return _dataBase.FindByGid (gid) != 0;
}

void PathFinder::MaterializeFolderPath (char const * path, bool quiet)
{
	bool isNetworkPath = FilePath::IsNetwork (path);
	FullPathSeq checkSeq (path);
	if (isNetworkPath && !File::Exists (checkSeq.GetHead ()))
	{
		throw Win::Exception ("Network path not accessible.", path);
	}
    // Count how many levels we have to create
    // If more then one ask the user for confirmation
	unsigned depth = File::CountMissingFolders (path);
    if (!quiet && depth > 1)
    {
		std::string info ("The path: '");
		info += path;
		info += "'\n\ndoes not exist. Do you want to create it?";
		Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
												   Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question),
												   TheAppInfo.GetWindow ());
        if (userChoice == Out::No)
            throw Win::Exception ();
    }

	if (depth != 0)
	{
		// Now materialize folder path
		File::MaterializePath (path);
	}
}

void PathFinder::CreateDirs ()
{
    // Check if project folder exists; if not create it
    MaterializeFolderPath (_baseDir.GetDir ());
    // Check if database folder exists; if not create it
    MaterializeFolderPath (GetSysPath ());
	// Clean it up if reusing
	ShellMan::DeleteContents (Win::Dow::Handle (), GetSysPath ());
    // Create mailboxes
	File::CreateFolder (_inBoxDir.GetDir ());
}

char const * PathFinder::GetFullPath (GlobalId fileGid, Area::Location loc)
{
    // PathFinder can return full path ONLY for file in project.
    // User Folder to get full path to files not in project.
    Assert (fileGid != gidInvalid);
    if (loc == Area::Project)
    {
        // Use database to construct full path in Project Area
        return _baseDir.MakePath (fileGid);
    }
    else
    {
        return GetAreaFullPath (fileGid, loc);
    }
}

char const * PathFinder::GetFullPath (UniqueName const & uname)
{
    // Get path to explicitly named project file
    // PathFinder can return full path ONLY for file in project.
    // User Folder to get full path to files not in project.
    if (uname.GetParentId () == gidInvalid && uname.GetName ().empty ())
	{
		// Return project root folder
		return _baseDir.GetDir ();
	}
    Assert (uname.GetParentId () != gidInvalid);
    return _baseDir.MakePath (uname);
}

char const * PathFinder::GetRootRelativePath (GlobalId fileGid) const
{
    // PathFinder can return project root relative path ONLY for file in project.
    Assert (fileGid != gidInvalid);
	return _rootRelDir.MakePath (fileGid);
}

char const * PathFinder::XGetRootRelativePath (GlobalId fileGid) const
{
	// PathFinder can return project root relative path ONLY for file in project.
	Assert (fileGid != gidInvalid);
	return _xRootRelDir.XMakePath (fileGid);
}

char const * PathFinder::GetRootRelativePath (UniqueName const & uname) const
{
    // PathFinder can return project root relative path ONLY for file in project.
    Assert (uname.GetParentId () != gidInvalid);
	return _rootRelDir.MakePath (uname);
}

char const * PathFinder::XGetFullPath (GlobalId fileGid, Area::Location loc)
{
    // PathFinder can return full path ONLY for file in project.
    // User Folder to get full path to files not in project.
    Assert (fileGid != gidInvalid);
    if (loc == Area::Project)
    {
        // Use database to construct full path in Project Area
        return _xBaseDir.XMakePath (fileGid);
    }
    else
    {
        Assert (loc != Area::OriginalBackup);
        return XGetAreaFullPath (fileGid, loc);
    }
}

char const * PathFinder::XGetFullPath (UniqueName const & uname)
{
    // Get path to explicitly named project file
    // PathFinder can return full path ONLY for file in project.
    // User Folder to get full path to files not in project.
    if (uname.GetParentId () == gidInvalid && uname.GetName ().empty ())
	{
		// Return project root folder
		return _baseDir.GetDir ();
	}
    Assert (uname.GetParentId () != gidInvalid);
    return _xBaseDir.XMakePath (uname);
}

char const * PathFinder::GetFileExtension (Area::Location loc, int orgAreaId) const
{
    switch (loc)
    {
    case Area::Original:
    case Area::OriginalBackup:
        if (orgAreaId == 1)
            return "og1";
        else if (orgAreaId == 2)
            return "og2";
    case Area::Reference:
        return "ref";
    case Area::Synch:
        return "syn";
    case Area::Staging:
        return "prj";
    case Area::PreSynch:
        return "bak";
	case Area::Temporary:
		return "tmp";
	case Area::Compare:
		return "cmp";
	case Area::LocalEdits:
		return "out";
    }
    Assert (!"Illegal file location");
    return "xxx";
}

char const * PathFinder::GetAllFilesPath (Area::Location loc) const
{
	std::string fileName ("*.");
    int orgAreaId = 0;
    if (loc == Area::Original)
        orgAreaId = _dataBase.GetOriginalId ();
    else if (loc == Area::OriginalBackup)
        orgAreaId = _dataBase.GetPrevOriginalId ();
    fileName += GetFileExtension (loc, orgAreaId);
    return GetSysFilePath (fileName.c_str ());
}

char const * PathFinder::XGetAllFilesPath (Area::Location loc) const
{
    Assert (loc != Area::OriginalBackup);
	std::string fileName ("*.");
    int orgAreaId = 0;
    if (loc == Area::Original)
        orgAreaId = _dataBase.XGetOriginalId ();
    fileName += GetFileExtension (loc, orgAreaId);
    return GetSysFilePath (fileName.c_str ());
}

char const * PathFinder::GetAreaFullPath (GlobalId fileGid, Area::Location loc) const
{
	std::ostringstream fileName;
    int orgAreaId = 0;
    if (loc == Area::Original)
        orgAreaId = _dataBase.GetOriginalId ();
    else if (loc == Area::OriginalBackup)
        orgAreaId = _dataBase.GetPrevOriginalId ();
    fileName << std::setw (8) << std::setfill ('0') << std::hex << fileGid << "." << GetFileExtension (loc, orgAreaId);
    return GetSysFilePath (fileName.str ().c_str ());
}

char const * PathFinder::XGetAreaFullPath (GlobalId fileGid, Area::Location loc) const
{
	std::ostringstream fileName;
    int orgAreaId = _dataBase.XGetOriginalId ();
    fileName << std::setw (8) << std::setfill ('0') << std::hex << fileGid << "." << GetFileExtension (loc, orgAreaId);
	return GetSysFilePath (fileName.str ().c_str ());
}
