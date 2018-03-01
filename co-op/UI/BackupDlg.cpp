//------------------------------------
//  (c) Reliable Software, 2007 - 2008
//------------------------------------

#include "precompiled.h"
#include "BackupDlg.h"
#include "CoopBlocker.h"
#include "BackupFileName.h"
#include "Global.h"

BackupRequest::BackupRequest (CoopBlocker & coopBlocker)
	: SaveFileRequest ("Create Code Co-op Backup Archive",
					   "Backup archive name:  ",
					   "Store the backup archive on:  ",
					   "Recent Backup",
					   "Backup FTP Site",
					   "Backup Path"),
	  _coopBlocker (coopBlocker)
{
	BackupFileName backupName;
	_fileName = backupName.Get ();
	_fileDescription = "backup archive";
}

bool BackupRequest::OnApply () throw ()
{
	return _coopBlocker.TryToHold (_quiet);	// Quiet if in command line mode
}

bool BackupControlHandler::GetDataFrom (NamedValues const & source)
{
	if (TheCurrentProductId != coopProId)
		throw Win::Exception ("Backup command line applet is not supported by this product version.");

	return SaveFileDlgCtrl::GetDataFrom (source);
}

RestoreRequest::RestoreRequest (CoopBlocker & coopBlocker)
	: OpenFileRequest ("Restore Code Co-op Projects From The Backup Archive",
					   "The backup archive is present on:  ",
					   "Open Code Co-op Backup Archive",
					   "Recent Restore",
					   "Restore FTP Site",
					   "Restore Path"),
	 _coopBlocker (coopBlocker)
{}

bool RestoreRequest::OnApply () throw ()
{
	return _coopBlocker.TryToHold (_quiet);	// Quiet if in command line mode
}

bool RestoreControlHandler::GetDataFrom (NamedValues const & source)
{
	if (TheCurrentProductId != coopProId)
		throw Win::Exception ("RestoreFromBackup command line applet is not supported by this product version.");

	return OpenFileDlgCtrl::GetDataFrom (source);
}
