#if !defined (COOPFINDER_H)
#define COOPFINDER_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2007
//------------------------------------

#include "FileClassifier.h"

#include <Win/Win.h>
#include <Win/EnumProcess.h>

// Predicate to be used with ProcessFinder

class IsInProject
{
public:
	// use projId == -1 to find co-op not in project
	IsInProject (char const * className, int projId)
		: _className (className), _projId (projId)
	{
		_actualClass.reserve (255);
	}
	void Init (char const * className, int projId)
	{
		_className = className;
		_projId = projId;
	}
	bool operator () (Win::Dow::Handle win);
private:
	std::string _actualClass;
	char const * _className;
	int _projId;
};

// Find Code Co-op visiting specified project

class CoopFinder
{
public:
	enum InstanceType
	{
		GuiCoopInProject,
		GuiCoopNotInProject,
		CoopServer,
		CoopNotFound
	};

public:
	CoopFinder ()
	{}
	InstanceType FindAnyCoop (FileListClassifier::ProjectFiles const * files);
	Win::Dow::Handle GetCoopWin () const { return _finder.GetWin (); }
private:
	Win::ProcessFinder<IsInProject> _finder;
};

#endif
