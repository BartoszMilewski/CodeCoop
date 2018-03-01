#if !defined (UNINSTALLER_H)
#define UNINSTALLER_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2005
//------------------------------------

#include <Sys/RegKey.h>
#include <File/Path.h>

class FilePath;

class Uninstaller
{
public:
	Uninstaller ();

	void PerformFullUninstall ();
	void UninstallUpdate ();

private:
	void DeleteFiles ();
	void DeleteIcons ();
	void DeleteAssociations ();
	void CleanupCatalog ();
	bool FindAndDelete (FilePath & curFolder);
	void CleanupLocalMachine ();
	void CleanupCurrentUser ();
	void CleanupSystem ();
	void DeleteKey (RegKey::Handle & thisKey, char const * thisKeyName, RegKey::Handle & parentKey);
	void CleanupCmdLineToolsFiles ();
	// temporary update
	void CheckBackupFiles ();
	void RestoreOriginalFiles ();
	void CleanupTemporaryInstall (bool careForVersionMarker = true);

private:
	FilePath	_installPath;
	FilePath	_backupPath;
	std::string _prevProvider;
	bool		_coopLinkDeleted;
	bool		_dispatcherLinkDeleted;
	bool		_uninstallLinkDeleted;
};

#endif
