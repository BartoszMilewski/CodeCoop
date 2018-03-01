//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "CoopFinder.h"
#include "CoopCaption.h"
#include "Global.h"

bool IsInProject::operator () (Win::Dow::Handle win)
{
	win.GetClassName (_actualClass);
	if (_actualClass == _className)
	{
		dbg << "IsInProject found class: " << _actualClass << std::endl;
		long id;
		if (win.GetProperty ("Project ID", id))
		{
			dbg << "Project id: " << id << "; searching for: " << _projId << std::endl;
			return id == _projId;
		}
		else
		{
			dbg << "No Project ID property" << std::endl;
			return _projId == -1;
		}
	}
	return false;
}

CoopFinder::InstanceType CoopFinder::FindAnyCoop (FileListClassifier::ProjectFiles const * files)
{
	CoopFinder::InstanceType instanceType = CoopNotFound;
	// Look for GUI Code Co-op in the specified project
	dbg << "	CoopFinder::FindAnyCoop" << std::endl;

	IsInProject pred (CoopClassName, files->GetProjectId ());

	if (!_finder.Find (pred))
	{
		pred.Init (ServerClassName, files->GetProjectId ());
		if (!_finder.Find (pred))
		{
			pred.Init (ServerClassName, -1);
			if (!_finder.Find (pred))
			{
				dbg << "	CoopServer not found" << std::endl;
				instanceType = CoopNotFound;
			}
			else
			{
				dbg << "	CoopServer found (not in project)" << std::endl;
				instanceType = CoopServer;
			}
		}
		else
		{
			dbg << "	CoopServer found in project " << std::endl;
			instanceType = CoopServer;
		}
	}
	else
	{
		dbg << "	GUI Coop found in project " << std::endl;
		instanceType = GuiCoopInProject;
	}
	return instanceType;
}
