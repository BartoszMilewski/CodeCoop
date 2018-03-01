#if !defined (SAVEFILESDLG_H)
#define SAVEFILESDLG_H
//------------------------------------
//  (c) Reliable Software, 2006 - 2008
//------------------------------------

#include "CopyRequest.h"
#include "Resource.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <File/Path.h>

#include <Bit.h>

class Catalog;

class SaveFilesData
{
public:
	SaveFilesData ();
	~SaveFilesData ();

	void SaveAfterChange (bool flag) { _options.set (AfterChange, flag); }
	void SetPerformDeletes (bool flag) { _options.set (PerformDeletesAtTarget, flag); }

	bool IsSaveAfterChange () const { return _options.test (AfterChange); }
	bool IsPerformDeletes () const { return _options.test (PerformDeletesAtTarget); }

	FileCopyRequest & GetFileCopyRequest () { return _copyRequest; }
	FileCopyRequest const & GetFileCopyRequest () const { return _copyRequest; }

	bool IsValid ();
	void DisplayErrors (Win::Dow::Handle hwndOwner) const;

private:
	enum SaveOptions
	{
		AfterChange,
		PerformDeletesAtTarget,
		TargetIsValid
	};

private:
	static char const	_preferencesCopyTypeValueName [];
	static char const	_preferencesFTPKeyName [];
	static char const	_preferencesLocalFolderValueName [];
	static char const	_preferencesUseFolderNames [];

private:
	FileCopyRequest		_copyRequest;
	BitSet<SaveOptions>	_options;
};

class SaveFilesCtrl : public Dialog::ControlHandler
{
public:
	SaveFilesCtrl (SaveFilesData & data)
		: Dialog::ControlHandler (IDD_SAVE_OLD_FILES),
		  _dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::RadioButton	_myComputer;
	Win::Edit			_localFolder;
	Win::Button			_browseMyComputer;
	Win::RadioButton	_internet;
	Win::Edit			_server;
	Win::Edit			_serverFolder;
	Win::CheckBox		_anonymousLogin;
	Win::Edit			_user;
	Win::Edit			_password;
	Win::CheckBox		_overwriteExisting;
	Win::CheckBox		_useFolderNames;
	Win::CheckBox		_performDeletes;
	Win::RadioButton	_versionBefore;
	Win::RadioButton	_versionAfter;
	SaveFilesData &		_dlgData;
};

#endif
