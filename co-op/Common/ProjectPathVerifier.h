#if !defined (PROJECTPATHVERIFIER_H)
#define PROJECTPATHVERIFIER_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "ProjectData.h"

class Catalog;
namespace Progress { class Meter; }
class ProjectSeq;

namespace Project
{
	// Goes over all projects testing the existence of root paths
	// If a root path doesn't exist and cannot be created,
	// asks user for new drive letter or network share
	// If the user cancels, the project gets "unavailable" status
	class RootPathVerifier
	{
	public:
		RootPathVerifier (Catalog & catalog);

		unsigned GetProjectCount () const { return _projectCount; }
		bool Verify (Progress::Meter & meter, bool skipUnavailable = true);
		void PromptForNew ();

	private:
		Catalog &					_catalog;
		unsigned					_projectCount;
		std::vector<Project::Data>	_projectsWithoutRootPath;
	};
}

#endif
