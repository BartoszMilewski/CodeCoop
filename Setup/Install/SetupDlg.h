#if !defined (SETUPDLG_H)
#define SETUPDLG_H
// ----------------------------------
// (c) Reliable Software, 1997 - 2009
// ----------------------------------

#include "InstallConfig.h"

#include <File/File.h>
#include <File/Path.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Ctrl/Static.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Controls.h>
#include <Win/Dialog.h>
#include <Win/Win.h>

// Windows WM_NOTIFY handler
class ProgramGroupNotifyHandler : public Notify::ListViewHandler
{
public:
	ProgramGroupNotifyHandler (unsigned ctrlId,
							   Win::ReportListing & pgmGroupList, 
							   Win::Edit & pgmGroup)
		: Notify::ListViewHandler (ctrlId), 
		  _pgmGroupList (pgmGroupList),
		  _pgmGroup (pgmGroup)
	{}
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
private:
	Win::ReportListing	& _pgmGroupList;
    Win::Edit			& _pgmGroup;
};

class SetupDlgCtrl : public Dialog::ControlHandler
{
public:
    SetupDlgCtrl (InstallConfig * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();

private:
    bool IsValid ();
    void DisplayErrors () const;

private:
    Win::Edit			_installPath;
    Win::Button			_browseInstall;
    Win::Edit			_pgmGroup;
	Win::ReportListing	_pgmGroupList;
	Win::CheckBox		_desktopShortcut;
	Win::CheckBox		_autoUpdate;

    InstallConfig	  *	_dlgData;
	bool				_canUseInstallationFolder;

	ProgramGroupNotifyHandler _pgmGroupHandler;
};

class DatabaseDlgData
{
public:
	DatabaseDlgData (FilePath const & proposedPath)
		: _path (proposedPath)
	{}
    bool Validate (bool quiet = false) const;
	void SetPath (std::string const & path) { _path.Change (path); }
	FilePath const & GetPath () const { return _path; }
private:
	FilePath	_path;
};

class DatabaseDlgCtrl: public Dialog::ControlHandler
{
public:
    DatabaseDlgCtrl (DatabaseDlgData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
    DatabaseDlgData *	_dlgData;
	Win::Button			_browse;
    Win::Edit			_path;
};

class CmdLineToolsDlgCtrl: public Dialog::ControlHandler
{
public:
    CmdLineToolsDlgCtrl (FilePath & installPath);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	FilePath &		_installPath;
	Win::Button		_browse;
    Win::Edit		_path;
};

class LicenseDlgData
{
public:
	LicenseDlgData (Win::Instance inst);
	char const * GetText () const throw () { return _text; }
private:
	char const * _text;
};

class LicenseDlgCtrl : public Dialog::ControlHandler
{
public:
    LicenseDlgCtrl (LicenseDlgData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::EditReadOnly   _licenseText;

    LicenseDlgData *	_dlgData;
};

class ConfirmDlgData
{
public:
	ConfirmDlgData (std::string const & infoFile, FilePath & installationFolder)
		: _runTutorial (true),
		  _infoFile (infoFile),
		  _installationFolder (installationFolder)
	{}

	void SetTutorial (bool flag) { _runTutorial = flag; }
	void ShowInfo () const;

	bool RunTutorial () const { return _runTutorial; }
	bool HasInfoFile () const { return File::Exists (_infoFile); }

private:
	bool				_runTutorial;
	std::string const &	_infoFile;
	FilePath &			_installationFolder;
};

class ConfirmDlgCtrl : public Dialog::ControlHandler
{
public:
    ConfirmDlgCtrl (ConfirmDlgData * data);

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
    bool OnApply () throw ();

private:
	Win::StaticText		_caption;
	Win::CheckBox		_tutorial;
	Win::Button			_showInfo;
    ConfirmDlgData *	_dlgData;
};

#endif
