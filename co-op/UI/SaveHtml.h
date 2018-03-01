#if !defined (SAVEHTML_H)
#define SAVEHTML_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <File/Path.h>

#include "Resource.h"

class Catalog;

class SaveHtmlData
{
public:
	SaveHtmlData (std::string const & curWikiDir);
	~SaveHtmlData ();

	void SetTargetFolder (std::string const & path) { _targetFolder.Change (path); }
	FilePath const & GetTargetFolder () const { return _targetFolder; }

	bool IsValid () const;
	void DisplayErrors (Win::Dow::Handle hwndOwner) const;
private:
	std::string	_currentWikiFolder;
	FilePath	_targetFolder;
};

class SaveHtmlCtrl : public Dialog::ControlHandler
{
public:
	SaveHtmlCtrl (SaveHtmlData & data)
		: Dialog::ControlHandler (IDD_EXPORT_HTML),
		  _dlgData (data)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::Edit			_targetFolder;
	Win::Button			_targetFolderBrowse;
	SaveHtmlData &		_dlgData;
};

#endif
