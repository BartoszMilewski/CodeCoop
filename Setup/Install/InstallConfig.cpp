// ----------------------------------
// (c) Reliable Software, 2005 - 2009
// ----------------------------------

#include "precompiled.h"
#include "InstallConfig.h"
#include "Registry.h"
#include "RegKeys.h"
#include "SetupRegistry.h"
#include "PathRegistry.h"
#include "AltDiffer.h"

InstallConfig::InstallConfig ()
	: _isPreviousInstallationDetected (false),
	  _prevIsUserSetup (false),
	  _isVersionLowerThen3 (false),
	  _newIsUserSetup (false),
	  _isDefaultInstallPath (true),
	  _isDesktopShortcut (true),
	  _isAutoUpdate (true),
	  _bcSupportsMerge(false)
{
	// Try detect version 3.0 or lower installation
	//
	// HKEY_CURRENT_USER
	//      Software
	//          Reliable Software
	//              Code Co-op
	//					Setup
	Registry::CoopSubKeyCheck coopSetup (true, "Setup"); // check user registry
	if (coopSetup.Exists ())
	{
		_isPreviousInstallationDetected = true;
		_prevIsUserSetup = true;
		_prevInstallPath = coopSetup.Key ().GetStringVal ("ProgramPath");
	}
	else
	{
		// HKEY_LOCAL_MACHINE
		//      Software
		//          Reliable Software
		//              Code Co-op
		//					Setup
		Registry::CoopSubKeyCheck coopSetup (false, "Setup"); // check machine registry
		if (coopSetup.Exists ())
		{
			_isPreviousInstallationDetected = true;
			_prevIsUserSetup = false;
			_prevInstallPath = coopSetup.Key ().GetStringVal ("ProgramPath");
		}
	}

	if (_isPreviousInstallationDetected)
	{
		// Check if installed version is lower then 3.0
		//
		// HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
		//		Software
		//			Reliable Software
		//				Code Co-op
		//					Projects
		Registry::CoopSubKeyCheck projectsKeyCheck (_prevIsUserSetup, "Projects");
		if (projectsKeyCheck.Exists ())
		{
			_isVersionLowerThen3 = true;
			return;
		}
	}
	else
	{
		// Try detect version 3.x or higher installation
		//
		// HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
		//		Software
		//			Reliable Software
		//				Code Co-op
		Registry::CoopKeyCheck coopUserCheck (true);	// Check HKEY_CURRENT_USER
		if (coopUserCheck.Exists ())
		{
			Registry::KeyCoopRo coopUserKey (true);
			std::string programPath = coopUserKey.GetProgramPath ();
			if (programPath.empty ())
			{
				Registry::CoopKeyCheck coopMachineCheck (false); // Check HKEY_LOCAL_MACHINE
				if (coopMachineCheck.Exists ())
				{
					Registry::KeyCoopRo coopMachineKey (false);
					programPath = coopMachineKey.GetProgramPath ();
					if (!programPath.empty ())
					{
						_isPreviousInstallationDetected = true;
						_prevIsUserSetup = false;
						_prevInstallPath = programPath;
						_prevCatalogPath = coopMachineKey.GetCatalogPath ();
					}
				}
			}
			else
			{
				_isPreviousInstallationDetected = true;
				_prevIsUserSetup = true;
				_prevInstallPath = programPath;
				_prevCatalogPath = coopUserKey.GetCatalogPath ();
			}
		}
	}

	_fullBeyondComparePath = ::RetrieveFullBeyondComparePath (_bcSupportsMerge);

	// Revisit:
	// find shortcut(s) to Dispatcher in startup folders
	// find shortcut(s) to Co-op in desktop folders
	// gather info about alt differ
	// the point is to preserve user settings concerning alt differ (which is not necessarily BC Differ)
	// most important: shortcuts, program group, differ
	// less important: window sizes, column widths, etc.


	// come up with defaults for new install
	_newInstallPath = _prevInstallPath;
	if (_prevInstallPath.IsDirStrEmpty ())
	{
		ProgramFilesPath sysPgmFilesPath;
	    FilePath installPath = sysPgmFilesPath;
		installPath.DirDown (CompanyName);
		installPath.DirDown (ApplicationName);
		_newInstallPath = installPath;
	}
	_newCatalogPath = _prevCatalogPath;
	_newIsUserSetup = _prevIsUserSetup;
}
