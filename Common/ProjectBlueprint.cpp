//----------------------------------
// (c) Reliable Software 2005 - 2009
//----------------------------------

#include "precompiled.h"
#include "ProjectBlueprint.h"
#include "PathRegistry.h"
#include "OutputSink.h"
#include "Catalog.h"
#include "Registry.h"

#include <Win/Dialog.h>	// For NamedValues

using namespace Project;

Blueprint::Blueprint (Catalog & catalog)
	: _catalog (catalog)
{
	std::string hubId (_catalog.GetHubId ());
	if (hubId.empty ())
	{
		Win::ClearError ();
		throw Win::Exception ("Code Co-op cannot create new project because the Dispatcher has not been properly configured.\n"
							  "Your Dispatcher hub id has not been set.  Run Dispatcher and set your hub id.");
	}
	else
	{
		_thisUser.SetHubId (hubId);
	}
	_thisUser.SetName (Registry::GetUserName ());
	// Read recent project path from the registry
	Registry::UserPreferences prefs;
	_project.SetRootPath (prefs.GetFilePath ("Recent Project Path"));
}

Blueprint::~Blueprint ()
{
	try
	{
		if (IsRootPathWellFormed (_project.GetRootPath ()) && IsProjectDefined ())
		{
			// Save project path in the registry
			PathSplitter splitter (_project.GetRootDir ());
			std::string recentPath (splitter.GetDrive ());
			recentPath += splitter.GetDir ();
			Registry::UserPreferences prefs;
			prefs.SaveFilePath ("Recent Project Path", recentPath);
		}
	}
	catch ( ... )
	{
		// Ignore all exceptions
	}
}

bool Blueprint::IsValid () const
{
    bool validBlueprint = IsRootPathOk () && IsProjectDefined ();
	if (validBlueprint)
	{
		if (_options.IsCheckProjectName ())
			return !_catalog.IsProjectNameUsed (_project.GetProjectName ());
		return true;
	}
	return false;
}

bool Blueprint::IsRootPathWellFormed (FilePath const & rootPath)
{
	char const * path = rootPath.GetDir ();
	FilePath const & catPath = Registry::GetCatalogPath ();
	SystemFilesPath sysPath;
	return !rootPath.IsDirStrEmpty ()					&&
			FilePath::IsValid (path)				&&
			FilePath::IsAbsolute (path)				&&
			FilePath::HasValidDriveLetter (path)	&&
		   !rootPath.HasPrefix (catPath)			&&
		   !catPath.HasPrefix (rootPath)			&&
		   !rootPath.HasPrefix (sysPath)			&&
		   !sysPath.HasPrefix (rootPath);
}

bool Blueprint::IsProjectDefined () const
{
    return !_project.GetProjectName ().empty () &&
		   !_thisUser.GetName ().empty () &&
		   !_thisUser.GetHubId ().empty ();
}

bool Blueprint::IsRootPathOk () const
{
	int projId;
	return IsRootPathWellFormed (_project.GetRootPath ()) &&
		   !_catalog.IsSourcePathUsed (_project.GetRootDir (), projId);
}

// Returns true when path errors displayed
bool Blueprint::DisplayPathErrors (FilePath const & rootPath, Win::Dow::Handle winOwner)
{
	if (rootPath.IsDirStrEmpty ())
	{
		TheOutput.Display ("Please, specify the project root folder (existing or not).",
						   Out::Information, winOwner);
		return true;
	}
	else
	{
		char const * sourcePath = rootPath.GetDir ();
		if (!FilePath::IsValid (sourcePath))
		{
			TheOutput.Display ("The project root folder path contains illegal characters.",
							   Out::Information, winOwner);
			return true;
		}
		else if (!FilePath::IsAbsolute (sourcePath))
		{
			TheOutput.Display ("Please, specify absolute path (local or UNC) "
							   "for the project root folder.",
							   Out::Information, winOwner);
			return true;
		}
		else if (!FilePath::HasValidDriveLetter (sourcePath))
		{
			TheOutput.Display ("Please select a valid drive (or a UNC path) "
							   "for the project root folder.",
							   Out::Information, winOwner);
			return true;
		}
		else
		{
			FilePath const & catPath = Registry::GetCatalogPath ();
			if (rootPath.HasPrefix (catPath))
			{
				std::string info ("Projects cannot be created inside the Code Co-op database folder:\n\n");
				info += catPath.GetDir ();
				TheOutput.Display (info.c_str (), Out::Information, winOwner);
				return true;
			}
			else if (catPath.HasPrefix (rootPath))
			{
				std::string info ("Projects root path cannot include the Code Co-op database folder:\n\n");
				info += catPath.GetDir ();
				TheOutput.Display (info.c_str (), Out::Information, winOwner);
				return true;
			}
			else
			{
				SystemFilesPath sysPath;
				if (rootPath.HasPrefix (sysPath))
				{
					std::string info ("Projects cannot be created inside Windows system folder:\n\n");
					info += sysPath.GetDir ();
					TheOutput.Display (info.c_str (), Out::Information, winOwner);
					return true;
				}
				else if (sysPath.HasPrefix (rootPath))
				{
					std::string info ("Project's root path cannot include the Windows system folder:\n\n");
					info += sysPath.GetDir ();
					TheOutput.Display (info.c_str (), Out::Information, winOwner);
					return true;
				}
			}
		}
	}
	return false;
}

void Blueprint::DisplayErrors (Win::Dow::Handle winOwner) const
{
	if (IsRootPathOk ())
	{
		if (_project.GetProjectName ().empty ())
		{
			TheOutput.Display ("Please, enter the project name.",
								Out::Information, winOwner);
		}
		else if (_options.IsCheckProjectName () && _catalog.IsProjectNameUsed (_project.GetProjectName ()))
		{
			std::string msg ("There already is a project named '");
			msg += _project.GetProjectName ();
			msg += "'.\n\nSelect another name for the project you are creating.";
			TheOutput.Display (msg.c_str (), Out::Information, winOwner);
		}
		else if (_thisUser.GetName ().empty ())
		{
			TheOutput.Display ("Please, enter your name.", Out::Information, winOwner);
		}
		else if (_thisUser.GetHubId ().empty ())
		{
			TheOutput.Display ("Please, enter the hub id.", Out::Information, winOwner);
		}
	}
	else
	{
		FilePath const & rootPath = _project.GetRootPath ();
		if (!DisplayPathErrors (rootPath, winOwner))
		{
			int projId;
			if (_catalog.IsSourcePathUsed (rootPath, projId))
			{
				if (_catalog.IsPathInProject (rootPath, projId))
				{
					TheOutput.Display ("This folder is already inside a Code Co-op project",
						Out::Information, winOwner);
				}
				else
				{
					TheOutput.Display ("This folder already contains a Code Co-op project.",
						Out::Information, winOwner);
				}
			}
		}
	}
}

std::string Blueprint::GetNamedValues ()
{
	// Project blueprint named values:
	//		project:"Name"
	//		root:"Local\Path"
	//		user:"My Name"
	//		userId:"id"
	//      email:"myHubId"
	//		comment:"my comment"
	//      autosynch:"yes"
	//		autojoin:"yes"
	//		autofullsynch:"yes"
	//		keepcheckedout:"yes"
	Assert (IsValid ());

	std::string namedValues ("project:\"");
	namedValues += _project.GetProjectName ();
	namedValues += "\" root:\"";
	namedValues += _project.GetRootDir ();

	namedValues += "\" user:\"";
	namedValues += _thisUser.GetName ();
	namedValues += "\" userId:\"";
	namedValues += _thisUser.GetUserId ();
	namedValues += "\" email:\"";
	namedValues += _thisUser.GetHubId ();
	namedValues += "\" comment:\"";
	namedValues += _thisUser.GetComment ();
	namedValues += "\"";

	if (_options.IsAutoSynch ())
		namedValues += " autosynch:\"yes\"";
	if (_options.IsAutoJoin ())
		namedValues += " autojoin:\"yes\"";
	if (_options.IsAutoFullSynch ())
		namedValues += " autofullsynch:\"yes\"";
	if (_options.IsKeepCheckedOut ())
		namedValues += " keepcheckedout:\"yes\"";

	return namedValues;
}

void Blueprint::ReadNamedValues (NamedValues const & input)
{
	_project.SetProjectName (input.GetValue ("project"));
	_project.SetRootPath (input.GetValue ("root"));

	std::string userName = input.GetValue ("user");
	if (!userName.empty ())
		_thisUser.SetName (userName);
	std::string userId = input.GetValue ("userId");
	if (!userId.empty ())
		_thisUser.SetUserId (userId);
	std::string myHubId (input.GetValue ("email"));
	if (!myHubId.empty ())
		_thisUser.SetHubId (myHubId);
	_thisUser.SetComment (input.GetValue ("comment"));

	_options.SetAutoSynch (input.GetValue ("autosynch") == "yes");
	_options.SetAutoJoin (input.GetValue ("autojoin") == "yes");
	_options.SetAutoFullSynch (input.GetValue ("autofullsynch") == "yes");
	_options.SetKeepCheckedOut (input.GetValue ("keepcheckedout") == "yes");
}

void Blueprint::Clear ()
{
	_project.Clear ();
	_options.Clear ();
	_thisUser.Clear ();
}
