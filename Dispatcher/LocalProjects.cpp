// ----------------------------------
// (c) Reliable Software, 2002 - 2008
// ----------------------------------

#include "precompiled.h"
#include "LocalProjects.h"
#include "SccProxyEx.h"
#include "ProjectMarker.h"
#include "Catalog.h"
#include <File/Dir.h>

LocalProjects::LocalProjects (Catalog & catalog)
	: _catalog (catalog), 
	  _areNewWithIncoming (false)
{
	_scriptPattern.push_back ("*.snc");
	_scriptPattern.push_back ("*.cnk");
}

void LocalProjects::Refresh ()
{
	_projectsToSync.clear ();
	_projectsWithIncoming.clear ();
	_projectsWithMissing.clear ();
	_areNewWithIncoming = false;
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		int projId = seq.GetProjectId ();
		Assert (projId > 0);
		FilePath projectInboxFolder (_catalog.GetProjectInboxPath (projId));
		// 1. examine contents of project inbox
		FileMultiSeq scriptSeq (projectInboxFolder, _scriptPattern);
		if (!scriptSeq.AtEnd ())
		{
			_projectsWithIncoming.insert (projId);
		}
		// 2. are there scripts already preprocessed by co-op waiting to be synched?
		IncomingScripts marker (_catalog, projId);
		if (marker.Exists ())
			_projectsToSync.insert (projId);
		// 3. are there missing scripts? (we need to notify co-op to generate resend requests)
		MissingScripts missMarker (_catalog, projId);
		if (missMarker.Exists ())
			_projectsWithMissing.insert (projId);
	}
	_areNewWithIncoming = !_projectsWithIncoming.empty ();
}

// return true, when project is added or deleted from the list
bool LocalProjects::OnStateChange (int projectId)
{
	Assert (projectId > 0);
	int prevProjCount = _projectsToSync.size ();
	IncomingScripts marker (_catalog, projectId);
	if (marker.Exists ())
		_projectsToSync.insert (projectId);
	else
		_projectsToSync.erase (projectId);

	MissingScripts missMarker (_catalog, projectId);
	if (missMarker.Exists ())
		_projectsWithMissing.insert (projectId);
	else
		_projectsWithMissing.erase (projectId);

	return prevProjCount != _projectsToSync.size ();
}

void LocalProjects::MarkNewIncomingScripts (int projectId)
{
	if (_projectsWithIncoming.insert (projectId).second)
		_areNewWithIncoming = true;
}

bool LocalProjects::UnpackIncomingScripts ()
{
	SccProxyEx coop;
	std::set<int>::iterator it = _projectsWithIncoming.begin ();
	while (it != _projectsWithIncoming.end ())
	{
		int projId = *it;
		Assert (projId > 0);
		if (coop.CoopCmd (projId, "RefreshMailbox", true, false)) // Skip GUI Co-op in the project
		{														  // Execute command with timeout
			// Remove project from the list only if there are no scripts in its Inbox folder 
			FilePath projectInboxFolder (_catalog.GetProjectInboxPath (projId));
			FileMultiSeq scriptSeq (projectInboxFolder, _scriptPattern);
			if (scriptSeq.AtEnd ())
			{
				it = _projectsWithIncoming.erase (it);
				continue;
			}
		}
		++it;
	}
	_areNewWithIncoming = false;
	return _projectsWithIncoming.empty ();
}

void LocalProjects::ResendMissingScripts ()
{
	SccProxyEx coop;
	for (std::set<int>::const_iterator it = _projectsWithMissing.begin ();
		it != _projectsWithMissing.end ();
		++it)
	{
		int projId = *it;
		dbg << "Dispatcher, Refresh Co-op Mailbox, project ID: " << projId << std::endl;
		Assert (projId > 0);
		coop.CoopCmd (projId, "RefreshMailbox", false, false); // Don't skip GUI co-op is in the project;
															   // Execute command with timeout
	}
}
