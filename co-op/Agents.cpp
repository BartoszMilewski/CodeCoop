//----------------------------------
// (c) Reliable Software 2004 - 2007
//----------------------------------

#include "precompiled.h"
#include "Agents.h"
#include "Permissions.h"
#include "Catalog.h"
#include "PathFind.h"
#include "ProjectDb.h"
#include "Registry.h"

bool ThisUserAgent::IsMyStateValid () const
{
	return _userPermissions.IsProjectDataValid ();
}

bool ThisUserAgent::IsReceiver () const
{
	return IsMyStateValid () ? _userPermissions.IsReceiver () : false;
}

void ThisUserAgent::XFinalize (Project::Db const & projectDb, Catalog & catalog, PathFinder & pathFinder, int curProjectId)
{
	// Perform membership update post-processing
	if (!_newHubId.empty ())
		catalog.RefreshProjMemberHubId (curProjectId, _newHubId);

	if (_descriptionChange)
	{
		std::unique_ptr<MemberDescription> currentDescription = projectDb.XRetrieveMemberDescription (projectDb.XGetMyId ());
		try
		{
			Registry::StoreUserDescription (*currentDescription);
		}
		catch (...)
		{
			Win::ClearError ();
			// Ignore all exceptions during catalog update
		}
	}

	_userPermissions.XRefreshProjectData (projectDb, &pathFinder);
}

void ThisUserAgent::XRefreshUserData (Project::Db const & projectDb)
{
	_userPermissions.XRefreshProjectData (projectDb);
}
