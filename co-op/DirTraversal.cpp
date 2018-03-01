//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "DirTraversal.h"
#include "Visitor.h"
#include "FileData.h"
#include "PathFind.h"

#include <File/Dir.h>
#include <File/Path.h>
#include <Ctrl/ProgressMeter.h>

Traversal::Traversal (Project::Path const & path, 
					  GlobalId rootId, 
					  Visitor & visitor, 
					  Progress::Meter & progressMeter)
    : _visitor (visitor),
	  _progressMeter (progressMeter)
{
    TraverseTree (path, rootId);
}

Traversal::Traversal (FilePath const & path, 
					  Visitor & visitor, 
					  Progress::Meter & progressMeter)
    : _visitor (visitor),
	  _progressMeter (progressMeter)
{
    TraverseTree (path, true);
}

Traversal::Traversal (PathFinder const & pathFinder, 
					  Area::Location loc, 
					  Visitor & visitor, 
					  Progress::Meter & progressMeter)
    : _visitor (visitor),
	  _progressMeter (progressMeter)
{
    TraverseArea (pathFinder, loc);
}

void Traversal::TraverseTree (Project::Path const & path, GlobalId parentId)
{
    Project::Path curPath (path);
    GlobalId folderId = gidInvalid;
    for (FileSeq seq (curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
    {
        char const * name = seq.GetName ();
		UniqueName uname (parentId, name);
        if (_visitor.Visit (uname , folderId))
        {
            // Visitor found folder in project and set folderId to its global id
            curPath.DirDown (name);
            TraverseTree (curPath, folderId);
            curPath.DirUp ();
        }
    }
}

void Traversal::TraverseTree (FilePath const & path, bool isRoot)
{
    FilePath curPath (path);
    for (FileSeq seq (curPath.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
    {
        char const * path = curPath.GetFilePath (seq.GetName ());
		_progressMeter.SetActivity (path);

        if (_visitor.Visit (path))
        {
            // Visitor found folder in project
            curPath.DirDown (seq.GetName ());
            TraverseTree (curPath, false);
            curPath.DirUp ();
			if (_progressMeter.WasCanceled ())
			{
				_visitor.CancelVisit ();
				return;
			}
			else if (isRoot)
			{
				_progressMeter.StepIt ();
			}
        }
    }
}

void Traversal::TraverseArea (PathFinder const & pathFinder, Area::Location loc)
{
    char const * allFilesInArea = pathFinder.GetAllFilesPath (loc);
    for (FileSeq seq (allFilesInArea); !seq.AtEnd (); seq.Advance ())
    {
        GlobalIdPack gid (seq.GetName ());
        _visitor.Visit (gid);
    }
}
