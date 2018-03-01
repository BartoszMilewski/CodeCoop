//----------------------------------
// (c) Reliable Software 2004 - 2009
//----------------------------------

#include "precompiled.h"
#include "PathClassifier.h"
#include "ProjectData.h"
#include "Catalog.h"

#include <File/File.h>
#include <File/Path.h>

PathClassifier::PathClassifier (unsigned int pathCount, char const **paths)
{
	// Find out which path really points to the Code Co-op project
	Catalog catalog;
	for (unsigned int i = 0; i < pathCount; ++i)
	{
		FilePath userPath (paths [i]);
		userPath.Canonicalize ();
		if (File::Exists (userPath.GetDir ()))
		{
			// Search Code Co-op catalog.
			ProjectSeq it (catalog);
			for (; !it.AtEnd (); it.Advance ())
			{
				Project::Data projectData;
				it.FillInProjectData (projectData);
				FilePath coopProjectRoot (projectData.GetRootDir ());
				Assert (!coopProjectRoot.IsDirStrEmpty ());
				if (userPath.HasPrefix (coopProjectRoot))
				{
					// User path points to some Code Co-op project -- remember its local id
					int projectId = projectData.GetProjectId ();
					if (std::find (_projectIds.begin (), _projectIds.end(), projectId) == _projectIds.end ())
						_projectIds.push_back (projectId);
					break;
				}
			}
			if (it.AtEnd ())
				_unrecognizedPaths.push_back (paths [i]);
		}
		else
		{
			_unrecognizedPaths.push_back (paths [i]);
		}
	}
}
