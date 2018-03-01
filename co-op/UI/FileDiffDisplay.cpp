//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "FileDiffDisplay.h"
#include "Image.h"
#include "FileDisplayTable.h"
#include "OutputSink.h"

#include <File/Path.h>
#include <Graph/Icon.h>
#include <Win/Keyboard.h>
#include <Sys/Clipboard.h>

FileDiffDisplay::FileDiffDisplay (unsigned ctrlId,
								  FileDisplayTable & fileTable, 
								  NotificationSink * notificationSink)
	: Notify::ListViewHandler (ctrlId),
	  _images (16, 16, imageLast),
	  _fileTable (fileTable),
	  _notificationSink (notificationSink),
	  _hideUnchanged (true)
{}

FileDiffDisplay::~FileDiffDisplay ()
{
	SetImageList ();
}

void FileDiffDisplay::Init (Win::Dow::Handle winParent, int id)
{
	Win::Report::Init (winParent, id);
	AddProportionalColumn (28, "File Name");
	AddProportionalColumn (36, "Path");
	AddProportionalColumn (12, "Global Id");
	AddProportionalColumn (10, "Type");
	AddProportionalColumn (10, "State");
	// Load images used in the regular file view
    for (int i = 0; i < imageLast; i++)
    {
		Icon::SharedMaker icon (16, 16);
		_images.AddIcon (icon.Load (winParent.GetInstance (), IconId [i]));
    }
    // Set overlay indices
    _images.SetOverlayImage (imageOverlaySynchNew, overlaySynchNew);
    _images.SetOverlayImage (imageOverlaySynchDelete, overlaySynchDelete);
    _images.SetOverlayImage (imageOverlaySynch, overlaySynch);
    _images.SetOverlayImage (imageOverlaySynchProj, overlaySynchProj);
	SetImageList (_images);
}

bool FileDiffDisplay::OnDblClick () throw ()
{
	OpenSelected ();
	return true;
}

bool FileDiffDisplay::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (_notificationSink != 0)
		_notificationSink->OnItemChanged (state);

	return true;
}

bool FileDiffDisplay::OnKeyDown (int key) throw ()
{
	if (key == VKey::Return || key == VKey::Control)
		OpenSelected ();

	return true;
}

void FileDiffDisplay::Refresh ()
{
	ClearRows ();
	GidSet const & files = _fileTable.GetFileSet ();
	if (files.empty ())
		return;
	for (GidSet::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
		GlobalId gid = *iter;
		Win::ListView::Item fileItem;
		fileItem.SetParam (gid);
		fileItem.SetText (_fileTable.GetFileName (gid));
		FileDisplayTable::ChangeState state = _fileTable.GetFileState (gid);
		if (state == FileDisplayTable::Unchanged && _hideUnchanged)
			continue;	// Skip unchanged files if not explicitly asked for

		int iconIdx = imageNone;
		int overlayIdx = overlayNone;
		switch (state)
		{
		case FileDisplayTable::Unchanged:
			iconIdx = imageCheckedIn;
			break;
		case FileDisplayTable::Changed:
			iconIdx = imageCheckedOut;
			break;
		case FileDisplayTable::Renamed:
			iconIdx = imageCheckedOut;
			break;
		case FileDisplayTable::Moved:
			iconIdx = imageCheckedOut;
			break;
		case FileDisplayTable::Deleted:
			iconIdx = imageDeleted;
			break;
		case FileDisplayTable::Created:
			iconIdx = imageNewFile;
			break;
		}
		if (_fileTable.IsFolder (gid))
			iconIdx = imageFolderIn;
		fileItem.SetIcon (iconIdx);
		fileItem.SetOverlay (overlayIdx);
		int itemPos = AppendItem (fileItem);

		PathSplitter splitter (_fileTable.GetRootRelativePath (gid));
		FilePath relPath (splitter.GetDir ());
        AddSubItem (relPath.GetDir (), itemPos, colPath);
		GlobalIdPack pack (gid);
        AddSubItem (pack.ToString (), itemPos, colGid);
		FileType type = _fileTable.GetFileType (gid);
		AddSubItem (type.GetName (), itemPos, colType);
		AddSubItem (FileDisplayTable::GetStateName (state).c_str (), itemPos, colState);
    }
	Select (0);
	SetFocus (0);
}

void FileDiffDisplay::OpenSelected ()
{
	GidList gids;
	try
	{
		for (SelectionIterator iter (*this); !iter.AtEnd (); iter.Advance ())
		{
			GlobalId gid = iter.GetGlobalId ();
			gids.push_back (gid);
		}
		_fileTable.Open (gids);
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
		_fileTable.Cleanup (gids);
		Win::ClearError ();
	}
	catch ( ... )
	{
		// Any exception during showing file differences cancels the show, but
		// doesn't close the dialog
		TheOutput.Display ("Cannot open selected files -- unknown error.");
		_fileTable.Cleanup (gids);
		Win::ClearError ();
	}
}

void FileDiffDisplay::OpenAll ()
{
	try
	{
		_fileTable.OpenAll ();
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
		Win::ClearError ();
	}
	catch ( ... )
	{
		// Any exception during showing file differences cancels the show, but
		// doesn't close the dialog
		TheOutput.Display ("Cannot open all files -- unknown error.");
		Win::ClearError ();
	}
}

void FileDiffDisplay::CopyToClipboard (Win::Dow::Handle dlgWin)
{
	std::string list;
	GidSet const & files = _fileTable.GetFileSet ();
	for (GidSet::const_iterator iter = files.begin (); iter != files.end (); ++iter)
    {
		GlobalId gid = *iter;
		FileDisplayTable::ChangeState state = _fileTable.GetFileState (gid);
		if (state == FileDisplayTable::Unchanged && _hideUnchanged)
			continue;	// Skip unchanged files if not explicitly asked for

		list += _fileTable.GetRootRelativePath (gid);
		list += "\n";
    }
	if (!list.empty ())
	{
		Clipboard clipboard (dlgWin);
		clipboard.PutText (list.c_str (), list.length ());
	}
}
