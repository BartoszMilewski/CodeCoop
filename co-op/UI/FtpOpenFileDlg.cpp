//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "FtpOpenFileDlg.h"
#include "Resource.h"
#include "OutputSink.h"

#include <Net/Ftp.h>
#include <StringOp.h>

FileListHandler::FileListHandler (FtpFileOpenCtrl & ctrl)
	: Notify::ListViewHandler (IDC_FTP_FOLDER_CONTENT),
	  _ctrl (ctrl)
{}

bool FileListHandler::OnDblClick () throw ()
{
	return _ctrl.OnDoubleClick ();
}

bool FileListHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.GainedSelection ())
		_ctrl.Select (state.Idx ());
	return true;
}

int const FtpFileOpenCtrl::_iconIds [imageLast] =
{
	I_FOLDER_IN,
	I_IN
};

FtpFileOpenCtrl::FtpFileOpenCtrl (FtpFileOpenData & dlgData, Win::Dow::Handle topWin)
	: Dialog::ControlHandler (IDD_FTP_OPEN_FILE),
#pragma warning (disable:4355)
	  _notifyHandler (*this),
#pragma warning (default:4355)
	  _fileImages (16, 16, imageLast),
	  _access ("Code Co-op"),
	  _dlgData (dlgData)
{
	for (int i = 0; i < imageLast; ++i)
	{
		Icon::SharedMaker icon (16, 16);
		_fileImages.AddIcon (icon.Load (topWin.GetInstance (), _iconIds [i]));
	}
}

Notify::Handler * FtpFileOpenCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_notifyHandler.IsHandlerFor (idFrom))
		return &_notifyHandler;
	else
		return 0;
}

bool FtpFileOpenCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_server.Init (dlgWin, IDC_FTP_SERVER);
	_folder.Init (dlgWin, IDC_FTP_CURRENT_FOLDER);
	_fileName.Init (dlgWin, IDC_FTP_CURRENT_FILE);
	_goUp.Init (dlgWin, IDC_FTP_GOUP);
	_content.Init (dlgWin, IDC_FTP_FOLDER_CONTENT);
	_content.AddProportionalColumn (45, "Name");
	_content.AddProportionalColumn (20, "Size", Win::Report::Right);
	_content.AddProportionalColumn (30, "Date Modified");
	_content.SetImageList (_fileImages);

	dlgWin.SetText (_dlgData.GetCaption ());
	_server.SetString (_dlgData.GetServer ());

	try
	{
		if (!_access.AttemptConnect ())
		{
			TheOutput.Display ("You are not connected to the Internet.");
			return false;
		}
		std::string url ("ftp://");
		url += _dlgData.GetServer ();
		if (!_access.CheckConnection (url.c_str (), true))
		{
			std::string info ("Cannot connect to ");
			info += url;
			TheOutput.Display (info.c_str ());
			return false;
		}

		_hInternet = _access.Open ();
		_session.SetInternetHandle (_hInternet);
		_session.SetServer (_dlgData.GetServer ());
		if (!_dlgData.IsAnonymousLogin ())
		{
			_session.SetUser (_dlgData.GetUser ());
			_session.SetPassword (_dlgData.GetPassword ());
		}
		_hFtp =  _session.Connect ();
	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException (e, dlgWin);
		return false;
	}
	catch ( ... )
	{
		TheOutput.Display ("Unknown exception during internet access.");
		return false;
	}

	SetCurrentFolder ();
	return true;
}

bool FtpFileOpenCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDC_FTP_GOUP && Win::SimpleControl::IsClicked (notifyCode))
	{
		_dlgData.DirUp ();
		SetCurrentFolder ();
		return true;
	}
	return false;
}

bool FtpFileOpenCtrl::OnApply () throw ()
{
	std::string fileName = _fileName.GetString ();
	if (fileName.empty ())
	{
		TheOutput.Display ("Please, select file");
		return false;
	}

	_dlgData.SetFileName (fileName);
	return Dialog::ControlHandler::OnApply ();
}

void FtpFileOpenCtrl::SetCurrentFolder ()
{
	std::string currentFolder = _dlgData.GetFolder ();
	if (currentFolder.empty ())
	{
		_goUp.Disable ();
		currentFolder = "/";
	}
	else
	{
		_goUp.Enable ();
		if (currentFolder [0] != '/')
			currentFolder.insert (0, 1, '/');
	}

	try
	{
		if (!_hFtp.SetCurrentDirectory (currentFolder.c_str ()))
		{
			std::string info ("The following folder cannot be found:\n\n");
			info += currentFolder;
			TheOutput.Display (info.c_str ());
			_goUp.Disable ();
			return;
		}

		_folder.SetString (currentFolder);
		_content.ClearRows ();
		_fileName.Clear ();

		// Folders
		for (Ftp::FileSeq seq (_hFtp, "*.*"); !seq.AtEnd (); seq.Advance ())
		{
			if (!seq.IsFolder ())
				continue;

			Win::ListView::Item item;
			item.SetText (seq.GetName ());
			item.SetParam (1);
			item.SetIcon (imageFolder);

			int row = _content.AppendItem (item);
			_content.AddSubItem ("", row, 1);				// Size
			_content.AddSubItem (PackedTimeStr (seq.GetWriteTime (), true).c_str (), row, 2);	// Date Modified
		}
		// Files
		for (Ftp::FileSeq seq (_hFtp, "*.*"); !seq.AtEnd (); seq.Advance ())
		{
			if (seq.IsFolder ())
				continue;

			Win::ListView::Item item;
			item.SetText (seq.GetName ());
			item.SetParam (0);
			item.SetIcon (imageFile);

			int row = _content.AppendItem (item);
			_content.AddSubItem (FormatFileSize (seq.GetSize ()).c_str (), row, 1);	// Size
			_content.AddSubItem (PackedTimeStr (seq.GetWriteTime (), true).c_str (), row, 2);	// Date Modified
		}

	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException (e, GetWindow ());
	}
	catch ( ... )
	{
		TheOutput.Display ("Unknown exception during internet access.");
	}
}

void FtpFileOpenCtrl::Select (int itemIdx)
{
	int itemParam = _content.GetItemParam (itemIdx);
	if (itemParam == 0)
	{
		std::string itemText = _content.RetrieveItemText (itemIdx);
		_fileName.SetString (itemText);
	}
}

bool FtpFileOpenCtrl::OnDoubleClick ()
{
	int itemIdx = _content.GetFirstSelected ();
	std::string itemText = _content.RetrieveItemText (itemIdx);
	int itemParam = _content.GetItemParam (itemIdx);
	if (itemParam != 0)
	{
		_dlgData.DirDown (itemText);
		SetCurrentFolder ();
	}
	else
	{
		_fileName.SetString (itemText);
		OnApply ();
	}
	return true;
}
