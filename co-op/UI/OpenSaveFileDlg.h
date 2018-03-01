#if !defined (OPENSAVEFILEDLG_H)
#define OPENSAVEFILEDLG_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "OpenSaveFileData.h"

#include <Win/Dialog.h>
#include <Ctrl/Button.h>
#include <Ctrl/Edit.h>
#include <File/Path.h>


class SaveFileDlgCtrl : public Dialog::ControlHandler
{
public:
	SaveFileDlgCtrl (SaveFileRequest & dlgData);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	Win::StaticText		_userNote;
	Win::StaticText		_fileNameFrame;
	Win::StaticText		_storeFrame;
	Win::Edit			_fileName;
	Win::Edit			_computerFolder;
	Win::Edit			_ftpServer;
	Win::Edit			_ftpFolder;
	Win::Edit			_ftpUser;
	Win::Edit			_ftpPassword;
	Win::Button			_browseComputer;
	Win::RadioButton	_myComputer;
	Win::RadioButton	_internet;
	Win::CheckBox		_overwriteExisting;
	Win::CheckBox		_ftpAnonymousLogin;

protected:
	SaveFileRequest &	_dlgData;
};

class OpenFileDlgCtrl : public Dialog::ControlHandler
{
public:
	OpenFileDlgCtrl (OpenFileRequest & dlgData, Win::Dow::Handle topWin);

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	Win::Dow::Handle	_topWin;
	Win::StaticText		_fileFrame;
	Win::Edit			_filePath;
	Win::Edit			_ftpServer;
	Win::Edit			_ftpFolder;
	Win::Edit			_ftpFilename;
	Win::Edit			_ftpUser;
	Win::Edit			_ftpPassword;
	Win::Button			_browseComputer;
	Win::RadioButton	_myComputer;
	Win::RadioButton	_internet;
	Win::CheckBox		_ftpAnonymousLogin;
	Win::Button			_browseFtp;

protected:
	OpenFileRequest &	_dlgData;
};

#endif
