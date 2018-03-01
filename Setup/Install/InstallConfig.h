#if !defined (INSTALL_CONFIG_H)
#define INSTALL_CONFIG_H
// ----------------------------------
// (c) Reliable Software, 2005 - 2008
// ----------------------------------

#include <File/Path.h>

class InstallConfig
{
	friend class SetupDlgCtrl;

public:
	InstallConfig ();

	FilePath const & GetPreviousInstallPath () const { return _prevInstallPath; }
	bool IsInSameFolderAsPrevious () const { return _prevInstallPath.IsEqualDir (_newInstallPath); }
	bool IsVersionLowerThen3 () const { return _isVersionLowerThen3; }
	bool PreviousInstallationDetected () const { return _isPreviousInstallationDetected; }

	FilePath const & GetInstallPath () const { return _newInstallPath; }
	FilePath const & GetCatalogPath () const { return _newCatalogPath; }
	std::string GetWikiDir () const { return _newCatalogPath.GetFilePath ("Wiki"); }
	void SetInstallPath (FilePath const & installPath) { _newInstallPath = installPath; }
	void SetCatalogPath (FilePath const & catPath)  { _newCatalogPath = catPath; }
	void SetIsUserSetup (bool isUserSetup) { _newIsUserSetup = isUserSetup; }
	bool IsUserSetup () const { return _newIsUserSetup; }

	FilePath const & GetFullBeyondCompareDifferPath () const { return _fullBeyondComparePath; }
	bool BcSupportsMerge() const { return _bcSupportsMerge; }
	std::string const & GetPgmGroupName () const { return _pgmGroupName; }
	bool IsDefaultInstallPath () const { return _isDefaultInstallPath; }
	bool IsDesktopShortcut () const { return _isDesktopShortcut; }
	bool IsAutoUpdate () const { return _isAutoUpdate; }

private:
	bool		_isPreviousInstallationDetected;
	FilePath	_prevInstallPath;
	FilePath	_prevCatalogPath;
	bool        _prevIsUserSetup;
	bool		_isVersionLowerThen3;

	FilePath	_newInstallPath;
	FilePath	_newCatalogPath;
	bool        _newIsUserSetup;

	bool		_bcSupportsMerge;
	FilePath	_fullBeyondComparePath;

    std::string	_pgmGroupName;
	bool		_isDefaultInstallPath;
	bool		_isDesktopShortcut;
	bool		_isAutoUpdate;
};

#endif
