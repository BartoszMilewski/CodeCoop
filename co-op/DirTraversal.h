#if !defined DIRTRAVERSAL_H
#define DIRTRAVERSAL_H
//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "GlobalId.h"
#include "Area.h"

class PathFinder;
namespace Project
{
	class Path;
}
class FilePath;
namespace Progress
{
	class Meter;
}
class Visitor;

class Traversal
{
public:
    Traversal (Project::Path const & path, 
				GlobalId rootId, 
				Visitor & visitor, 
				Progress::Meter & progressMeter);
	Traversal (FilePath const & path, 
				Visitor & visitor, 
				Progress::Meter & progressMeter);
    Traversal (PathFinder const & pathFinder, 
				Area::Location loc, 
				Visitor & visitor, 
				Progress::Meter & progressMeter);

private:
    void TraverseTree (Project::Path const & path, GlobalId parentId);
	void TraverseTree (FilePath const & path, bool isRoot);
    void TraverseArea (PathFinder const & pathFinder, Area::Location loc);

private:
    Visitor &		_visitor;
	Progress::Meter & _progressMeter;
};

#endif
