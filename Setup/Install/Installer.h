#if !defined (INSTALLER_H)
#define INSTALLER_H
//------------------------------------
//  (c) Reliable Software, 1997 - 2009
//------------------------------------

#include "InstallConfig.h"
#include "UndoFiles.h"

#include <Win/Win.h>
#include <Sys/Process.h>

class Catalog;
class MemberDescription;
class FilePath;
namespace Progress
{
	class Meter;
}
namespace Project
{
	class Data;
}
namespace Win
{
	class MessagePrepro;
}

class Installer
{
public:
	Installer (Win::MessagePrepro & msgPrepro,
			   Win::Dow::Handle	app)
		: _win (app),
		  _msgPrepro (msgPrepro)
	{}
	~Installer ();

	bool PerformInstallation (Progress::Meter & meter);
	bool TemporaryUpdate ();
	bool PermanentUpdate ();
	bool InstallCmdLineTools ();
	void ConfirmInstallation ();
	unsigned SelectGlobalLicense (Progress::Meter & meter);

private:
	bool PostInstallTask ();
	std::string GetShareName (std::string const & publicInboxShare);
	void CheckForCurrentCoopInstall ();
	void VerifyFiles (char const * const fileList [], FilePath const & folder, char const * errMsg);
	void VerifySetup ();
	FilePath QueryDatabasePath (FilePath const & dbPath);
	void BackupOriginalFiles ();
	void CleanupTemporaryInstall ();
	void CleanupPreviousInstallation ();
	void TransferFiles ();
	void CopyFiles (char const * const fileList []);
	void CopyUpdateFiles (FilePath const & destPath);
	void CreateIcons ();
	void AssociateOpenCommand (std::string const & extension,
							   std::string const & extensionClass,
							   std::string const & associationDescription,
							   std::string const & programName);
	void CreateAssociations ();
	void CreateDatabaseFolders (Progress::Meter & meter);
	void SetupCatalog ();
	void SetupRegistry ();
	void ConvertEmailRegistry ();
	void RemovePreferencesIfNecessary (std::string const & keyName);
	void ConvertToVersion4 (Catalog & catalog);
	void ConvertProjects (Catalog & catalog, Progress::Meter & meter);
	void CleanUpProjects ();
	bool ValidateProjectRegistryData (Project::Data const & projData, 
									  MemberDescription const & member) const;
	bool ValidatePath (FilePath const & path) const;
	bool MaterializeFolderPath (char const * path, bool askUser, bool quiet = false);
	void StoreLicense ();
	void ConfigDefaultDifferMerger ();
	bool UpdatePathVariable (std::string & pathVar, std::string const & installPath);
	void UpdateEnvironmentVariables ();

private:

	Win::Dow::Handle	_win;
	Win::MessagePrepro &_msgPrepro;

	InstallConfig		_config;
	UndoTransferFiles   _undoFileList;
	std::unique_ptr<Win::ChildProcess>	_externalTool;
	std::string			_infoFile;
	std::string			_toolOutputFile;
};

#endif
