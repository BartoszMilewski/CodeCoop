//------------------------------------
//	(c) Reliable Software, 1999 - 2007
//------------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "ProjectTable.h"
#include "Global.h"
#include "Catalog.h"
#include "History.h"
#include "Registry.h"
#include "ProjectState.h"
#include "ProjectMarker.h"

#include <Dbg/Assert.h>
#include <File/Dir.h>

DegreeOfInterest Project::IsInteresting (Catalog const & catalog, int projectId)
{
	IncomingScripts incomingScripts (catalog, projectId);
	if (incomingScripts.Exists ())
		return VeryInteresting;

	AwaitingFullSync awaitingFullsync (catalog, projectId);
	if (awaitingFullsync.Exists ())
		return Interesting;

	CheckedOutFiles checkedOutFiles (catalog, projectId);
	if (checkedOutFiles.Exists ())
		return Interesting;

	RecoveryMarker recovery (catalog, projectId);
	if (recovery.Exists ())
		return Interesting;

	return NotInteresting;
}

bool Project::Dir::FolderChange (FilePath const & folder)
{
	ExternalNotify ();
	return true;
}

bool Project::Dir::IsEmpty () const
{
	ProjectSeq seq (_catalog);
	return seq.AtEnd ();
}

DegreeOfInterest Project::Dir::HowInteresting () const
{
	DegreeOfInterest degree = NotInteresting;
	ProjectSeq seq (_catalog);
	while (!seq.AtEnd ())
	{
		switch (Project::IsInteresting (_catalog, seq.GetProjectId ()))
		{
		case VeryInteresting:
			return VeryInteresting;
			break;
		case Interesting:
			degree = Interesting;
			break;
		}
		seq.Advance ();
	}
	Assert (seq.AtEnd ());
	return degree;
}

// Table interface

void Project::Dir::QueryUniqueIds (Restriction const & restrict, GidList & ids) const
{
	if (restrict.HasIds ())
	{
		// Restriction has pre-selected ids
		GidList const & preSelectedIds = restrict.GetPreSelectedIds ();
		std::copy (preSelectedIds.begin (), preSelectedIds.end (), std::back_inserter (ids));
		return;
	}

	GidList allProjects;
	bool interestingOnly = restrict.IsOn ("InterestingOnly");
	for (ProjectSeq seq (_catalog); !seq.AtEnd (); seq.Advance ())
	{
		int id = seq.GetProjectId ();
		allProjects.push_back (id);
		if (interestingOnly)
		{
			if (Project::IsInteresting (_catalog, id) != NotInteresting)
				ids.push_back (id);
		}
	}
	if (interestingOnly)
	{
		Registry::RecentProjectList recentProjects;
		for (GidList::const_iterator it = recentProjects.begin ();
			 it != recentProjects.end ();
			 ++it)
		{
			unsigned int currProject = *it;
			if (std::find (ids.begin (), ids.end (), currProject) == ids.end () &&
				std::find (allProjects.begin (), allProjects.end (), currProject) != allProjects.end ())
			{
				ids.push_back (currProject);
			}
		}
		GidList::iterator curProj = std::find (ids.begin (), ids.end (), _curProjId);
		if (curProj != ids.end ())
			ids.erase (curProj);
	}
	else
	{
		ids.swap (allProjects);
	}
}

std::string Project::Dir::GetStringField (Column col, GlobalId gid) const
{
	if (col == colName)
		return _catalog.GetProjectName (gid);
	else if (col == colStateName)
		return _catalog.GetProjectSourcePath (gid).GetDir ();
	else
		return std::string ();
}

unsigned long Project::Dir::GetNumericField (Column col, GlobalId gid) const
{
	Assert (col == colState);
	CheckedOutFiles checkedOutFiles (_catalog, gid);
	IncomingScripts incomingScripts (_catalog, gid);
	AwaitingFullSync awaitingFullSync (_catalog, gid);
	RecoveryMarker recovery (_catalog, gid);
	Project::State state;
	state.SetMail (incomingScripts.Exists ());
	state.SetFullSync (awaitingFullSync.Exists ());
	state.SetCheckedoutFiles (checkedOutFiles.Exists ());
	state.SetCurrent (_curProjId == gid);
	state.SetUnavailable (_catalog.IsProjectUnavailable (gid));
	state.SetUnderRecovery (recovery.Exists ());
	return state.GetValue ();
}

bool Project::Dir::GetBinaryField (Column col, void * buf, unsigned int bufLen, GlobalId gid) const
{
	if (col == colTimeStamp && bufLen >= sizeof (FileTime))
	{
		FilePath dataBase = _catalog.GetProjectDataPath (gid);
		FileSeq seq (dataBase.GetFilePath (History::Db::GetCmdLogName ()));
		memcpy (buf, &seq.GetWriteTime (), sizeof (FileTime));
		return true;
	}
	return false;
}
