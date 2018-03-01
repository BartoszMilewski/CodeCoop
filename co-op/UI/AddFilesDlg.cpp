//------------------------------------
//  (c) Reliable Software, 1998 - 2005
//------------------------------------

#include "precompiled.h"
#include "AddFilesDlg.h"
#include "FolderContents.h"
#include "resource.h"

#include <Ctrl/Output.h>
#include <Graph/Canvas.h>

// Windows WM_NOTIFY handler for extension list view

ExtensionListHandler::ExtensionListHandler (AddFilesCtrl & ctrl)
	: Notify::ListViewHandler (IDC_ADD_FILES_EXTENSION), _ctrl (ctrl)
{}

bool ExtensionListHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.GainedSelection ())
	{
		_ctrl.ShowExtension (state.Idx ());
	}
	else if (state.GetNewStateImageIdx () != state.GetOldStateImageIdx ())
	{
		_ctrl.ChangeExtensionSelection (state.Idx ());
	}
	return true;
}

Notify::Handler * AddFilesCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_notifyHandler.IsHandlerFor (idFrom))
		return &_notifyHandler;
	else
		return 0;
}

AddFilesCtrl::AddFilesCtrl (FolderContents * contents)
	: Dialog::ControlHandler (IDD_ADD_FILES),
	  _contents (contents),
	  _curExtensionItem (-1),
	  _initFileSelectionCount (0),
	  _ignoreNotifications (false),
#pragma warning (disable:4355)
	  _notifyHandler (*this)
#pragma warning (default:4355)
{}

bool AddFilesCtrl::OnInitDialog () throw (Win::Exception)
{
	_count.Init (GetWindow (), IDC_ADD_FILES_COUNT);
	_extensionFrame.Init (GetWindow (), IDC_ADD_FILES_EXT_FRAME);
	_extensionView.Init (GetWindow (), IDC_ADD_FILES_EXTENSION);
	_fileView.Init (GetWindow (), IDC_ADD_FILES_FILE);
	_selectAllExt.Init (GetWindow (), IDC_ADD_FILES_SELECT_ALL_EXT);
	_deselectAllExt.Init (GetWindow (), IDC_ADD_FILES_DESELECT_ALL_EXT);
	_selectAllFiles.Init (GetWindow (), IDC_ADD_FILES_SELECT_ALL_FILE);
	_deselectAllFiles.Init (GetWindow (), IDC_ADD_FILES_DESELECT_ALL_FILE);

	_extensionView.AddCheckBoxes ();
	_contents->GetExtensionList (_extensionList);
	// Find the longest file path and set appropriate horizontal extend for scrolling
	std::vector<std::string>::iterator iter;
	int maxPixelWidth = 0;
	int cx, cy;
	Win::UpdateCanvas listBoxCanvas (_fileView);
	for (iter = _extensionList.begin (); iter != _extensionList.end (); ++iter)
	{
		FolderContents::FileSeq seq = _contents->GetFileList (*iter);
		for (; !seq.AtEnd (); seq.Advance ())
		{
			listBoxCanvas.GetTextSize (seq.GetPath ().c_str (), cx, cy);
			if (cx > maxPixelWidth)
			{
				maxPixelWidth = cx;
			}
		}
	}
	_fileView.SetHorizontalExtent (maxPixelWidth);
	// Display file name extension list
	for (unsigned int i = 0; i < _extensionList.size (); ++i)
	{
		std::string displayExt ("*");
		std::string const & ext = _extensionList [i];
		if (!ext.empty ())
			displayExt += ext.c_str ();
		else
			displayExt += ".";
		_extensionView.AppendItem (displayExt.c_str (), i);
	}
	// Display files with first extension on the list
	Assert (_contents->GetSelectedCount () == 0);
	_extensionView.DeSelectAll ();
	_extensionView.Select (0);
	_extensionView.SetFocus (0);
	SetCount ();
	return true;
}

bool AddFilesCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_ADD_FILES_FILE:
		if (_fileView.HasSelChanged (notifyCode))
		{
			int fileSelectionCount = _fileView.GetSelCount ();
			if (_extensionView.IsChecked (_curExtensionItem))
			{
				if (fileSelectionCount == 0)
					_extensionView.Uncheck (_curExtensionItem);
			}
			else
			{
				// Remember selection
				std::vector<int> selection;
				_fileView.RetrieveSelection (selection);
				_extensionView.Check (_curExtensionItem);
				// Checking selection will select all files -- correct selection
				unsigned int extIdx = _extensionView.GetItemParam (_curExtensionItem);
				Assert (extIdx < _extensionList.size ());
				_contents->SelectFiles (_extensionList [extIdx], selection);
				_initFileSelectionCount = DisplaySelection ();
			}
			SetCount ();
			return true;
		}
		break;
	case IDC_ADD_FILES_SELECT_ALL_EXT:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			// Checking all extensions will change current extension
			// Remember current extension.
			_contents->SelectAll ();
			_ignoreNotifications = true;
			_extensionView.UncheckAll ();
			_extensionView.CheckAll ();
			_ignoreNotifications = false;
			ChangeExtensionSelection (_curExtensionItem);
			_extensionView.SetFocus ();
			SetCount ();
			return true;
		}
		break;
	case IDC_ADD_FILES_DESELECT_ALL_EXT:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			// Checking all extensions will change current extension
			// Remember current extension.
			_contents->DeSelectAll ();
			_ignoreNotifications = true;
			_extensionView.UncheckAll ();
			_ignoreNotifications = false;
			ChangeExtensionSelection (_curExtensionItem);
			_extensionView.SetFocus ();
			SetCount ();
			return true;
		}
		break;
	case IDC_ADD_FILES_SELECT_ALL_FILE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_extensionView.Check (_curExtensionItem);
			_extensionView.SetFocus ();
			SetCount ();
			return true;
		}
		break;
	case IDC_ADD_FILES_DESELECT_ALL_FILE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			_extensionView.Uncheck (_curExtensionItem);
			_extensionView.SetFocus ();
			SetCount ();
			return true;
		}
		break;
	}
    return false;
}

bool AddFilesCtrl::OnApply () throw ()
{
	RememberCurrentSelection ();
	EndOk ();
	return true;
}

bool AddFilesCtrl::OnCancel () throw ()
{
	_contents->DeSelectAll ();
	EndCancel ();
	return true;
}

void AddFilesCtrl::ShowExtension (int item)
{
	if (_ignoreNotifications)
		return;

	if (_curExtensionItem != item)
	{
		RememberCurrentSelection ();
		// Change current extension and its file list
		_curExtensionItem = item;
		DisplayFiles ();
	}
	_initFileSelectionCount = DisplaySelection ();
}

void AddFilesCtrl::ChangeExtensionSelection (int item)
{
	if (_ignoreNotifications)
		return;

	// Change extension selection -- make it the current extension
	// so the file list change
	_extensionView.DeSelect (_curExtensionItem);
	unsigned int extIdx = _extensionView.GetItemParam (item);
	Assert (extIdx < _extensionList.size ());
	_contents->SetExtensionSelection (_extensionList [extIdx], _extensionView.IsChecked (item));
	_extensionView.Select (item);
	_extensionView.SetFocus (item);
	SetCount ();
}

void AddFilesCtrl::SetCount ()
{
	// Display selected file number
	int count = _contents->GetSelectedCount ();
	int currentFileSelectionCount = _fileView.GetSelCount ();
	if (currentFileSelectionCount >= _initFileSelectionCount)
		count += currentFileSelectionCount - _initFileSelectionCount;
	else
		count -= _initFileSelectionCount - currentFileSelectionCount;

	std::string info;
	if (count == 0)
	{
		info += "Nothing selected";
	}
	else
	{
		info += "Total: ";
		info += ToString (count);
		if (count == 1)
			info += " file";
		else
			info += " files";
		info += " selected";
	}
	_count.SetText (info.c_str ());
}

void AddFilesCtrl::DisplayFiles ()
{
	if (!_extensionList.empty ())
	{
		_fileView.Empty ();
		unsigned int extIdx = _extensionView.GetItemParam (_curExtensionItem);
		Assert (extIdx < _extensionList.size ());
		FolderContents::FileSeq seq = _contents->GetFileList (_extensionList [extIdx]);
		unsigned int fileCount = 0;
		for ( ; !seq.AtEnd (); seq.Advance (), ++fileCount)
		{
			_fileView.AddItem (seq.GetPath ().c_str ());
			if (seq.IsSelected ())
			{
				_fileView.Select (fileCount);
			}
		}
		std::string info ("Showing files ");
		std::string const & ext = _extensionList [extIdx];
		if (ext.empty ())
		{
			info += "without extension. ";
		}
		else
		{
			info += "with extension '";
			info += ext;
			info += "'. ";
		}
		info += "There ";
		if (fileCount == 1)
		{
			info += "is ";
			info += ToString (fileCount);
			info += " file";
		}
		else
		{
			info += "are ";
			info += ToString (fileCount);
			info += " files";
		}
		if (ext.empty ())
			info += " without extension. ";
		else
			info += " with this extension.";
		_extensionFrame.SetText (info.c_str ());
	}
}

int AddFilesCtrl::DisplaySelection ()
{
	int selectedFileCount = 0;
	_fileView.DeSelectAll ();
	if (!_extensionList.empty ())
	{
		unsigned int extIdx = _extensionView.GetItemParam (_curExtensionItem);
		Assert (extIdx < _extensionList.size ());
		std::string const & ext = _extensionList [extIdx];
		selectedFileCount = _contents->GetSelectedFileCount (ext);
		if (selectedFileCount != 0)
		{
			int fileCount = _contents->GetFileCount (ext);
			if (selectedFileCount == fileCount)
			{
				_fileView.SelectAll ();
			}
			else
			{
				FolderContents::FileSeq seq = _contents->GetFileList (ext);
				for (unsigned int i = 0 ; !seq.AtEnd (); seq.Advance (), ++i)
				{
					if (seq.IsSelected ())
					{
						_fileView.Select (i);
					}
				}
			}
		}
	}
	return selectedFileCount;
}

void AddFilesCtrl::RememberCurrentSelection ()
{
	// Remember current extension selected files
	unsigned int extIdx = _extensionView.GetItemParam (_curExtensionItem);
	Assert (extIdx < _extensionList.size ());
	std::vector<int> selection;
	_fileView.RetrieveSelection (selection);
	_contents->SelectFiles (_extensionList [extIdx], selection);
}
