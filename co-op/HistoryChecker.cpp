//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "precompiled.h"
#undef DBG_LOGGING
#define DBG_LOGGING false
#include "HistoryChecker.h"
#include "History.h"
#include "SccProxyEx.h"
#include "Catalog.h"
#include "OutputSink.h"

GlobalId HistoryChecker::IsRelatedProject (int projectId) const
{
	dbg << "--> IsRelatedProject" << std::endl;
	Assert (projectId != -1);
	// Note: fork ids are always returned in the reverse chronological order
	GlobalId otherProjectYoungestForkId = gidInvalid;
	GidList otherProjectYoungerForkIds;
	GidList myForkIds;
	dbg << "    GetForkIds" << std::endl;
	_history.GetForkIds (myForkIds);
	SccProxyEx proxy;
	dbg << "    SccProxyEx::GetForkScriptIds" << std::endl;
	if (proxy.GetForkScriptIds (projectId,
								false,	// Regular fork ids
								myForkIds,
								otherProjectYoungestForkId,
								otherProjectYoungerForkIds))
	{
		dbg << "    got ForkScriptIds" << std::endl;
		// Check if we have any of the target younger fork ids
		if (!otherProjectYoungerForkIds.empty ())
		{
			GidList myYoungerForkIds;
			dbg << "    History::CheckForkIds" << std::endl;
			GlobalId youngerForkIdFound = _history.CheckForkIds (otherProjectYoungerForkIds,
																 false,	// Regular fork ids
																 myYoungerForkIds);
			if (youngerForkIdFound != gidInvalid)
				return youngerForkIdFound;
		}
	}

	if (otherProjectYoungestForkId != gidInvalid)
		return otherProjectYoungestForkId;

	// Try matching deep fork ids
	myForkIds.clear ();
	otherProjectYoungerForkIds.clear ();
	dbg << "    History::GetForks" << std::endl;
	_history.GetForkIds (myForkIds, true);	// Get deep fork ids
	dbg << "    proxy GetForks" << std::endl;
	if (proxy.GetForkScriptIds (projectId,
								true,	// Deep fork ids
								myForkIds,
								otherProjectYoungestForkId,
								otherProjectYoungerForkIds))
	{
		// Check if we have any of the target younger fork ids
		if (!otherProjectYoungerForkIds.empty ())
		{
			GidList myYoungerForkIds;
			dbg << "    History::CheckForkIds" << std::endl;
			GlobalId youngerForkIdFound = _history.CheckForkIds (otherProjectYoungerForkIds,
																 true,	// Deep fork ids
																 myYoungerForkIds);
			if (youngerForkIdFound != gidInvalid)
				return youngerForkIdFound;
		}
	}
	dbg << "<-- IsRelatedProject" << std::endl;

	return otherProjectYoungestForkId;
}

void HistoryChecker::DisplayError (Catalog & catalog, int thisProjectId, int selectedProjectId) const
{
	std::string targetName (catalog.GetProjectName (selectedProjectId));
	std::string info ("Current project ");
	info += catalog.GetProjectName (thisProjectId);
	info += " appears to be unrelated to:\n\n";
	info += targetName;
	info += " (";
	info += catalog.GetProjectSourcePath (selectedProjectId).GetDir ();
	info += ")\n\n"
		"This may be because one of the projects has too short a history.\n"
		"Import a longer history from another project member\n"
		"and then try to merge the projects again.";
	TheOutput.Display (info.c_str ());
}