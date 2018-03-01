// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "precompiled.h"
#include "BrowseForFolder.h"
#include "Registry.h"

#include <Com/Shell.h>
#include <StringOp.h>

bool BrowseForLocalFolder (std::string & path,
						   Win::Dow::Handle parentWin,
						   std::string const & hintForTheUser,
						   char const * startupFolder)
{
	ShellMan::DrivesPath root;
	std::string dlgCaption = parentWin.GetText ();
	ShellMan::FolderBrowser folder (parentWin,
									root,
									hintForTheUser.c_str (),
									dlgCaption.empty () ? 0 : dlgCaption.c_str (),
									startupFolder);

	if (folder.IsOK ())
	{
		path = folder.GetPath ();
		return true;
	}

	return false;
}

bool BrowseForNetworkFolder (std::string & path,
							 Win::Dow::Handle parentWin,
							 std::string const & hintForTheUser,
							 char const * startupFolder)
{
	ShellMan::NetworkPath root;
	std::string dlgCaption = parentWin.GetText ();
	ShellMan::FolderBrowser folder (parentWin,
									root,
									hintForTheUser.c_str (),
									dlgCaption.empty () ? 0 : dlgCaption.c_str (),
									startupFolder);

	if (folder.IsOK ())
	{
		path = folder.GetPath ();
		return true;
	}

	return false;
}

bool BrowseForAnyFolder (std::string & path,
						 Win::Dow::Handle parentWin,
						 std::string const & hintForTheUser,
						 char const * startupFolder)
{
	ShellMan::VirtualDesktopFolder root;
	std::string dlgCaption = parentWin.GetText ();
	ShellMan::FolderBrowser folder (parentWin,
									root,
									hintForTheUser.c_str (),
									dlgCaption.empty () ? 0 : dlgCaption.c_str (),
									startupFolder);

	if (folder.IsOK ())
	{
		path = folder.GetPath ();
		return true;
	}

	return false;
}

bool BrowseForProjectRoot (std::string & path, Win::Dow::Handle owner, bool isCreating)
{
	// Read recent project path from the registry
	Registry::UserPreferences prefs;
	std::string hintForTheUser ("Select folder (existing or not) for the ");
	if (isCreating)
		hintForTheUser += "new project you are creating.";
	else
		hintForTheUser += "project you are joining.";
	// Project root folder can be local or network folder
	return BrowseForAnyFolder (path,
							   owner,
						       hintForTheUser.c_str (),
							   prefs.GetFilePath ("Recent Project Path").c_str ());
}

bool BrowseForSatelliteShare (std::string & path, Win::Dow::Handle owner)
{
	return BrowseForNetworkFolder (path,
								   owner,
								   "Select the appropriate machine and its CODECOOP share.");
}

bool BrowseForHubShare (std::string & path, Win::Dow::Handle owner)
{
	return BrowseForNetworkFolder (path,
								   owner,
								   "Find the hub machine and select its CODECOOP share.");
}
