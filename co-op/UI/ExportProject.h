#if !defined (EXPORTPROJECT_H)
#define EXPORTPROJECT_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2008
//------------------------------------

#include "VersionInfo.h"
#include "CopyRequest.h"

#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>

class Catalog;

class ExportProjectData
{
public:
	ExportProjectData (VersionInfo & info, Catalog & catalog);
	~ExportProjectData ();

	void SetIncludeLocalEdits (bool flag) { _options.set (IncludeLocalEdits, flag); }

	FileCopyRequest & GetFileCopyRequest () { return _copyRequest; }
	FileCopyRequest const & GetFileCopyRequest () const { return _copyRequest; }

	void SetVersionId (GlobalId scriptId) { _versionInfo.SetVersionId (scriptId); }
	GlobalId GetVersionId () const { return _versionInfo.GetVersionId (); }
	std::string GetVersionComment () const;
	long GetVersionTimeStamp () const { return _versionInfo.GetTimeStamp (); }
	bool IsCurrentVersion () const { return _versionInfo.IsCurrent (); }

	bool IsIncludeLocalEdits () const { return _options.test (IncludeLocalEdits); }

	bool IsValid ();
	void DisplayErrors (Win::Dow::Handle hwndOwner) const;

private:
	enum ExportOptions
	{
		IncludeLocalEdits,
		TargetIsValid
	};

private:
	static char const	_preferencesCopyTypeValueName [];
	static char const	_preferencesFTPKeyName [];
	static char const	_preferencesLocalFolderValueName [];

private:
	FileCopyRequest			_copyRequest;
	VersionInfo &			_versionInfo;
	Catalog	&				_catalog;
	BitSet<ExportOptions>	_options;

};

class ExportProjectCtrl : public Dialog::ControlHandler
{
public:
    ExportProjectCtrl (ExportProjectData & exportData);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	Win::StaticText		_versionFrame;
	Win::Edit			_version;
	Win::RadioButton	_myComputer;
	Win::Edit			_localFolder;
	Win::Button			_browseMyComputer;
	Win::RadioButton	_internet;
	Win::Edit			_server;
	Win::Edit			_serverFolder;
	Win::CheckBox		_anonymousLogin;
	Win::Edit			_user;
	Win::Edit			_password;
	Win::CheckBox		_localEdits;
	Win::CheckBox		_overwriteExisting;
    ExportProjectData &	_exportData;
};

#endif
