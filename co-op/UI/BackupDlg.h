#if !defined (BACKUPDLG_H)
#define BACKUPDLG_H
//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "OpenSaveFileDlg.h"

class CoopBlocker;

class BackupRequest : public SaveFileRequest
{
public:
	BackupRequest (CoopBlocker & coopBlocker);

	bool OnApply () throw ();

private:
	CoopBlocker &	_coopBlocker;
};

class BackupControlHandler : public SaveFileDlgCtrl
{
public:
	BackupControlHandler (BackupRequest & dlgData)
		: SaveFileDlgCtrl (dlgData)
	{}

	bool GetDataFrom (NamedValues const & source);
};

class RestoreRequest : public OpenFileRequest
{
public:
	RestoreRequest (CoopBlocker & coopBlocker);

	char const * GetFileFilter () const { return "Backup Archive (*.cab)\0*.cab\0All Files (*.*)\0*.*"; }
	bool OnApply () throw ();

private:
	CoopBlocker &	_coopBlocker;
};

class RestoreControlHandler : public OpenFileDlgCtrl
{
public:
	RestoreControlHandler (RestoreRequest & dlgData, Win::Dow::Handle topWin)
		: OpenFileDlgCtrl (dlgData, topWin)
	{}

	bool GetDataFrom (NamedValues const & source);
};

#endif
