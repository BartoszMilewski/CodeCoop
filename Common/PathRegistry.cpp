//----------------------------------
// (c) Reliable Software 2000 - 2008
//----------------------------------

#include "precompiled.h"
#include "PathRegistry.h"
#include "RegKeys.h"

#include <File/Path.h>
#include <File/File.h>

namespace Registry
{
	enum SetupType { setupUnknown, setupUser, setupMachine };

	static SetupType currentSetupType = setupUnknown;

	bool IsUserSetup ()
	{
		if (currentSetupType == setupUnknown)
		{
			Registry::CoopKeyCheck check (true);
			if (check.Exists ())
			{
				Registry::KeyCoopRo userKey (true);
				std::string path = userKey.GetProgramPath ();
				if (path.empty ())
					currentSetupType = setupMachine;
				else
					currentSetupType = setupUser;
			}
			else
				currentSetupType = setupMachine;
		}
		return currentSetupType == setupUser;
	}

	std::string GetProgramPath ()
	{
		Registry::KeyCoopRo key (IsUserSetup ());
		std::string path = key.GetProgramPath ();
		if (!File::Exists (path))
			throw Win::ExitException ("Code Co-op installation path doesn't exists!\n"
									  "Run Code Co-op installation again.", path.c_str ());
		return path;
	}

	std::string GetCatalogPath ()
	{
		Registry::KeyCoopRo key (IsUserSetup ());
		std::string path = key.GetCatalogPath ();
		if (!File::Exists (path))
			throw Win::ExitException ("Code Co-op catalog path doesn't exists!\n"
									  "Run Code Co-op installation again.", path.c_str ());
		return path;
	}

	std::string GetDatabasePath ()
	{
		Registry::KeyCoopRo key (IsUserSetup ());
		FilePath databaseFolder (key.GetCatalogPath ());
		databaseFolder.DirDown ("Database");
		return databaseFolder.ToString ();
	}

	std::string GetCmdLineToolsPath ()
	{
		Registry::KeyCoopRo key (IsUserSetup ());
		std::string path = key.GetCmdLineToolsPath ();
		if (!path.empty () && !File::Exists (path))
			throw Win::ExitException ("Code Co-op command line applet installation path doesn't exists!\n"
									  "Run Code Co-op installation again.", path.c_str ());
		return path;
	}

	std::string GetLogsFolder ()
	{
		try
		{
			Registry::KeyCoopRo key (IsUserSetup ());
			FilePath logsFolder (key.GetCatalogPath ());
			logsFolder.DirDown ("Logs");
			return logsFolder.ToString ();
		}
		catch (Win::ExitException ex)
		{
			throw ex;
		}
		catch ( ... )
		{
			Win::ClearError ();
		}
		return std::string ();
	}
}