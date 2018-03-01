//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"

#undef DBG_LOGGING
#define DBG_LOGGING false

#include "Commander.h"
#include "AppHelp.h"
#include "OutputSink.h"
#include "BackupFile.h"
#include "EditorPool.h"
#include "FileNames.h"
#include "EditParams.h"
#include "Lines.h"
#include "Diff.h"
#include "Merge.h"
#include "LineCounter.h"
#include "AboutDlg.h"
#include "Registry.h"
#include "PathRegistry.h"
#include "SccOptions.h"
#include "SccProxy.h"
#include "GoToLineDlg.h"
#include "TabDlg.h"
#include "Resource.h"

#include <Ctrl/FileGet.h>
#include <Ctrl/FontSelector.h>
#include <Ctrl/MultiProgressDialog.h>
#include <File/MemFile.h>
#include <Sys/Clipboard.h>
#include <Com/Shell.h>
#include <Sys/WinString.h>
#include <Win/Message.h>
#include <Ex/WinEx.h>
#include <Dbg/Out.h>
#include <auto_array.h>

Commander::Commander (Win::Dow::Handle mainWnd,
			   Win::Dow::Handle & viewWin,
			   Win::Dow::Handle & focusWin,
			   EditorPool & editorPool,
			   FileNames & fileNames,
			   Win::MessagePrepro & msgPrepro)
	: _mainWin (mainWnd),
	  _viewWin (viewWin),
	  _diffWin (editorPool.GetDiffWindow ()),
	  _focusWin (focusWin),
	  _editorPool (editorPool),
	  _fileNames (fileNames),
	  _msgPrepro (msgPrepro),
	  _leftPaneValid (false),
	  _rightPaneValid (false),
	  _findPrompter (0),
	  _tabs (0)
{}


void Commander::Program_ExitSave ()
{
	if (SaveFile ())
	{
		Win::UserMessage msg (UM_EXIT);
		_mainWin.PostMsg (msg);
	}
}

void Commander::Program_Exit ()
{
	if (CheckForEditChanges ())
	{
		Win::UserMessage msg (UM_EXIT);
		_mainWin.PostMsg (msg);
	}
}

void Commander::Program_About ()
{
    HelpAboutData dlgData (_fileNames);
	HelpAboutCtrl ctrl (dlgData);
    Dialog::Modal helpAbout (_mainWin, ctrl);
}

//
// File commands
//

// Revisit using XML
void Commander::File_Open ()
{
	if (CheckForEditChanges ())
	{
		FileGetter  fileDlg;
		CurrentFolder curFolder;
		fileDlg.SetInitDir (curFolder.GetDir ());
        fileDlg.SetFilter ("All Files (*.*)\0*.*\0"
						   "Source File (*.c *.cpp *.cxx)\0*.c;*.cpp;*.cxx\0"
						   "Header File (*.h *.hpp *.hxx *.inl)\0*.h;*.hpp;*.hxx;*.inl\0"
				           "Other Text (*.txt *.def *.mak *.rc)\0*.txt;*.def;*.mak;*.rc");
        if (fileDlg.GetExistingFile (_mainWin, "Open File"))
        {
			Win::UserMessage msgPlacement (UM_UPDATE_PLACEMENT);
			_mainWin.SendMsg (msgPlacement);
			_fileNames.Clear ();
			_fileNames.SetProjectFile (fileDlg.GetPath ());
			InitViewing ();
			Win::Dow::Handle mainWnd (_mainWin);
			SetTitle(_fileNames.Project ());
        }
	}
}

Cmd::Status Commander::can_File_Open () const
{
	return Cmd::Enabled;
}

void Commander::File_New ()
{
	if (CheckForEditChanges ())
	{   // save old placement
		Win::UserMessage msgPlacement (UM_UPDATE_PLACEMENT);
		_mainWin.SendMsg (msgPlacement);
		// cleaning
		Win::UserMessage msgBegin (UM_CLEAR_EDIT_STATE);
	    _viewWin.SendMsg (msgBegin);
		_fileNames.Clear ();
		OpenNewFile ();
	}
}

Cmd::Status Commander::can_File_New () const
{
	return Cmd::Enabled;
}

void Commander::File_Save ()
{
	SaveFile ();
}

Cmd::Status Commander::can_File_Save () const
{
	return NeedsSave () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::File_SaveAs ()
{
	// Get currently displayed file size
	int newSize = 0;
	unsigned int getSizeAlways = 1;
	Win::UserMessage msg (UM_EDIT_NEEDS_SAVE, getSizeAlways, reinterpret_cast<long>(&newSize));
	_viewWin.SendMsg (msg);

	FileGetter  fileDlg;
	CurrentFolder curFolder;
	fileDlg.SetInitDir (curFolder.GetDir ());
	fileDlg.SetFileName (_fileNames.Project ().c_str ());
	if (fileDlg.GetNewFile (_mainWin, "Save As"))
	{
		WriteEditBufferToFile (_viewWin, fileDlg.GetPath (), newSize);
		if (_fileNames.IsSingleFile ())
		{
			_fileNames.Clear ();
			_fileNames.SetProjectFile (fileDlg.GetPath ());
			InitTabs ();
			Win::Dow::Handle mainWnd (_mainWin);
			SetTitle(_fileNames.Project ());
			int code = 0;
			if (IsDualPaneDisplay ())
			{
				_leftPaneValid = true;
				_rightPaneValid = true;
			}
			else
			{
				_leftPaneValid = true;
				_rightPaneValid = false;
				code = 2; //_rightPaneValid = false
			} 
			// notify top level
			Win::UserMessage msgH (UM_HIDE_WINDOW, code);
			_mainWin.PostMsg (msgH);
		}
	}
}

Cmd::Status Commander::can_File_SaveAs () const
{
	return Cmd::Enabled;
}

void Commander::File_Refresh ()
{
	File_Save ();
	Win::UserMessage msgPlacement (UM_UPDATE_PLACEMENT);
	_mainWin.SendMsg (msgPlacement);
	_fileNames.GetSccStatus (true);	// Force Scc status refresh
	_fileNames.VerifyPaths ();
	InitViewing (true); // true refresh
}

Cmd::Status Commander::can_File_Refresh () const
{
	return  (_fileNames.HasFiles () && !_fileNames.HasPath (FileAfter))
		? Cmd::Enabled : Cmd::Disabled; 
}

void Commander::File_CheckOut ()
{
	Assert (_fileNames.IsControlledFile ());
	CodeCoop::Proxy proxy;
	char const * path = _fileNames.Project ().c_str ();
	CodeCoop::SccOptions options;
	if (proxy.CheckOut (1, &path, options))
	{
		if (_fileNames.GetProjectFileReadOnlyState ())
		{
			// Change from read-only to read-write
			Win::UserMessage msg (UM_SET_EDIT_READONLY);
			_viewWin.SendMsg (msg);
		}
		_fileNames.MakeProjectFileCheckedout ();
	}
}

Cmd::Status Commander::can_File_CheckOut () const
{
	if (_fileNames.ProjectIsPresent ())
	{
		return _fileNames.IsReadOnlyFile () &&
			   _fileNames.IsControlledFile ()? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::File_UncheckOut ()
{
	Assert (_fileNames.IsCheckedoutFile ());

	Out::Answer userChoice = TheOutput.Prompt (
		"Are you sure you want to restore changed files to their previous state?\n"
		"All local edits, file renames, type changes, etc. you've made since check-out will be lost.",
		Out::PromptStyle (Out::YesNo, Out::No, Out::Question));
	if (userChoice != Out::Yes)
		return ;

	Win::UserMessage msgPlacement (UM_UPDATE_PLACEMENT);
	_mainWin.SendMsg (msgPlacement);	
	CodeCoop::Proxy proxy;
	CodeCoop::SccOptions options;
	char const * path = _fileNames.Project ().c_str ();
	if (proxy.UncheckOut (1, &path, options))
	{
		if (_fileNames.GetProjectFileReadOnlyState ())
		{
			// Change from read-write to read-only
			Win::UserMessage msg (UM_SET_EDIT_READONLY, 1);
			_viewWin.SendMsg (msg);
		}
#if 0
		if (_differInfo.IsLocalRename ())
			_fileNames.SetProjectFile (_differInfo.GetLocalPrevName ().c_str ());
		else if (_differInfo.IsLocalMove ())
			_fileNames.SetProjectFile (_differInfo.GetLocalPrevLocation ().c_str ());
#endif
		_fileNames.VerifyPaths ();
		_fileNames.MakeProjectFileControlled ();
		_fileNames.GetProjectFileTimeStamp ();
		_fileNames.TestBeforeFile (); // Likely disappeared
		InitViewing (true); // true refresh
	}
}

Cmd::Status Commander::can_File_UncheckOut () const
{
	if (_fileNames.ProjectIsPresent () && !_fileNames.IsReadOnlyFile ())
	{
		return _fileNames.IsCheckedoutFile () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::File_CompareWith ()
{	
	FileGetter  fileDlg;
	CurrentFolder curFolder;
	fileDlg.SetInitDir (curFolder.GetDir ());
    fileDlg.SetFilter ("All Files (*.*)\0*.*\0"
						"Source File (*.c *.cpp *.cxx)\0*.c;*.cpp;*.cxx\0"
						"Header File (*.h *.hpp *.hxx *.inl)\0*.h;*.hpp;*.hxx;*.inl\0"
				        "Other Text (*.txt *.def *.mak *.rc)\0*.txt;*.def;*.mak;*.rc");
    if (fileDlg.GetExistingFile (_mainWin, "Open File"))
    {
		Win::UserMessage msgPlacement (UM_UPDATE_PLACEMENT);
		_mainWin.SendMsg (msgPlacement);
		std::string comparingFile;
		_fileNames.SetFileForComparing (fileDlg.GetPath ());

		// Get currently displayed file size

		unsigned int getSizeAlways = 1;
		int sizeBuf = 0;
		Win::UserMessage msg1 (UM_EDIT_NEEDS_SAVE, getSizeAlways, reinterpret_cast<long>(&sizeBuf));
		_viewWin.SendMsg (msg1);

		auto_array<char> buf (sizeBuf);
		unsigned int noSave = 1;
		Win::UserMessage msg2 (UM_GET_BUF, noSave, reinterpret_cast<long>(&buf [0]));
		_viewWin.SendMsg (msg2);
		if (msg2.GetResult () != sizeBuf)
			throw Win::Exception ("Internal Error: Cannot fill save buffer");
		{
			MemFileReadOnly beforeFile (fileDlg.GetPath ());
			File::Size fileSize = beforeFile.GetSize ();
			if (fileSize.IsLarge ())
				throw Win::Exception ("File size exceeds 4GB", fileDlg.GetPath (), 0);

			LineBuf lineBuf (beforeFile.GetBuf (), fileSize.Low ());
			LineCounter counter;
			std::unique_ptr<EditBuf> editBuf (new EditBufTarget);
			Progress::Meter progress;
			lineBuf.Dump (*editBuf, counter, progress);
			_editorPool.AddDocument (FileBefore, std::move(editBuf));
		}
		Compare (fileDlg.GetPath (), &buf [0], sizeBuf, EditStyle::chngUser);
		int code = 0; // both panes valid
		Win::UserMessage msg (UM_HIDE_WINDOW, code);
		_mainWin.PostMsg (msg);
		InitTabs ();
	}
}

Cmd::Status Commander::can_File_CompareWith () const
{
	return Cmd::Enabled;
}

//
//	Edit commands
//

void Commander::Edit_Undo ()
{
	if (_focusWin != _viewWin)
	{
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_viewWin);
		_mainWin.SendMsg (msg);
		_focusWin = _viewWin;
	}		
	Win::UserMessage msg (UM_UNDO);
	int numberAction = 1;
	msg.SetLParam (numberAction);
	_viewWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Undo () const
{
	Win::UserMessage msg (UM_CAN_UNDO);
	_viewWin.SendMsg (msg);
	if (msg.GetResult () == 1)
		return Cmd::Enabled;
	return Cmd::Disabled;
}

void Commander::Edit_Redo ()
{
	if (_focusWin != _viewWin)
	{
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_viewWin);
		_mainWin.SendMsg (msg);
		_focusWin = _viewWin;
	}
	Win::UserMessage msg (UM_REDO);
	int numberAction = 1;
	msg.SetLParam (numberAction);
	_viewWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Redo () const
{
	Win::UserMessage msg (UM_CAN_REDO);
	_viewWin.SendMsg (msg);
	if (msg.GetResult () == 1)
		return Cmd::Enabled;
	return Cmd::Disabled;
}

void Commander::Edit_Copy ()
{
	Win::UserMessage msg (UM_EDIT_COPY);
	_focusWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Copy () const
{
	return Cmd::Enabled;
}

void Commander::Edit_Cut ()
{
	Win::UserMessage msg (UM_EDIT_CUT);
	_viewWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Cut () const
{
	if (IsFocusOnEditable ())
	{
		return Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::Edit_Paste ()
{ 
	Win::UserMessage msg (UM_EDIT_PASTE);
	_viewWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Paste () const
{
	if (IsFocusOnEditable ())
	{
		// Read/write file in edit window -- check if clipboard contains text
		Clipboard clipboard (_mainWin);
		return clipboard.HasText () ? Cmd::Enabled : Cmd::Disabled;
	}
	return Cmd::Disabled;
}

void Commander::Edit_Delete ()
{ 
	Win::UserMessage msg (UM_EDIT_DELETE);
	_viewWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_Delete () const
{
	if (IsFocusOnEditable ())
	{
		return Cmd::Enabled;
	}
	return Cmd::Disabled;
}

void Commander::Edit_SelectAll ()
{
	Win::UserMessage msg (UM_SELECT_ALL);
	_focusWin.SendMsg (msg);
}

Cmd::Status Commander::can_Edit_SelectAll () const
{
	Win::UserMessage msg (UM_GETDOCDIMENSION, SB_VERT);
	_focusWin.SendMsg (msg);
	return msg.GetResult () != 0 ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Edit_ChangeFont ()
{
	Font::Selector fontSelector;

	fontSelector.SetFixedPitchOnly ();
	fontSelector.SetScreenFontsOnly ();
	fontSelector.LimitSizeSelection (6, 24);
	Font::Maker regFont;
	unsigned tabSize = 4;
	Registry::UserDifferPrefs prefs;
	if (prefs.GetFont (tabSize, regFont))
		fontSelector.SetDefaultSelection (regFont);
	else
		fontSelector.SetDefaultSelection ("Courier New", 8);
	if (fontSelector.Select ())
	{
		_editorPool.FontChange (tabSize, fontSelector.GetSelection ());
		prefs.RememberFont (tabSize, fontSelector.GetSelection ());
	}
}

Cmd::Status Commander::can_Edit_ChangeFont () const
{
	return Cmd::Enabled;
}

void Commander::Edit_ChangeTab ()
{
	Registry::UserDifferPrefs prefs;
	unsigned tabSizeChar = prefs.GetTabSize ();
	if (tabSizeChar == 0)
		tabSizeChar = 4;
	TabSizeCtrl ctrl (tabSizeChar);
	Dialog::Modal dlg (_mainWin, ctrl);
	if (dlg.IsOK ())
	{
		_editorPool.SetTabSize (tabSizeChar);
		prefs.SetTabSize (tabSizeChar);
	}
}

void Commander::Edit_ToggleLineBreaking ()
{
	_editorPool.ToggleLineBreaking ();
}

Cmd::Status Commander::can_Edit_ToggleLineBreaking () const
{
	return _editorPool.IsLineBreakingOn () ? Cmd::Checked : Cmd::Enabled;
}


//
// Search commands
// 

void Commander::Search_NextChange ()
{
	if (_focusWin == _viewWin)
	{
		// When searching focus has to be in diff window
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_diffWin);
		_mainWin.SendMsg (msg);
	}
	Win::UserMessage msg (UM_SEARCH_NEXT);
	_diffWin.SendMsg (msg);
}

Cmd::Status Commander::can_Search_NextChange () const
{
	return _rightPaneValid ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Search_PrevChange ()
{
	if (_focusWin == _viewWin)
	{
		// When searching focus has to be in diff window
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_diffWin);
		_mainWin.SendMsg (msg);
	}
	Win::UserMessage msg (UM_SEARCH_PREV);
	_diffWin.SendMsg (msg);
}

Cmd::Status Commander::can_Search_PrevChange () const
{
	return _rightPaneValid ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Search_FindText ()
{
	_findPrompter->InitDialogFind (_focusWin);
}

Cmd::Status Commander::can_Search_FindText () const
{
	return Cmd::Enabled;
}

void Commander::Search_FindNext ()
{
	_findPrompter->FindNext ();	
}

void Commander::Search_FindPrev ()
{
	_findPrompter->FindNext (true); // directionBackward = true
}

Cmd::Status Commander::can_Search_FindNext () const
{
	return Cmd::Enabled;
}

void Commander::Search_Replace ()
{
	_findPrompter->InitDialogReplace (_focusWin);
}

Cmd::Status Commander::can_Search_Replace () const
{
	return IsFocusOnEditable () ? Cmd::Enabled : Cmd::Disabled;
}

void Commander::Search_GoToLine ()
{
	// read the current paragraph
	Win::UserMessage msgCurrPos (UM_GETDOCPOSITION);
	msgCurrPos.SetWParam (SB_VERT);
    _focusWin.SendMsg (msgCurrPos);
    int curParaNo = msgCurrPos.GetResult () + 1;

   // read the last paragraph no
    Win::UserMessage msgLastPara (UM_GET_DOC_SIZE);
	_focusWin.SendMsg (msgLastPara);
	int lastParaNo = msgLastPara.GetResult ();
	//	size might be 0, but in that case we allow user to go to line 1
	lastParaNo = std::max (lastParaNo, 1);

	// initialize dialog
	GoToLineDlgData _dlgData (curParaNo, lastParaNo);
    GoToLineDlgController ctrl (_dlgData);
    Dialog::Modal goToDlg (_mainWin, ctrl);
	
	// go to selected paragraph
	if (goToDlg.IsOK ())
	{
		Win::UserMessage msgGoTo (UM_GOTO_PARAGRAPH);
		msgGoTo.SetLParam (_dlgData.GetParaNo ());
		_mainWin.SendMsg (msgGoTo);
	}
}

Cmd::Status Commander::can_Search_GoToLine () const
{
	return Cmd::Enabled;
}

//
// Help commands
//

void Commander::Help_Contents ()
{
	AppHelp::Display (AppHelp::DifferTopic, "help", _focusWin);
}

//
// Navigation commands
//

void Commander::Nav_NextPanel ()
{
	if (_focusWin == _diffWin)
	{
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_viewWin);
		_mainWin.SendMsg (msg);
		_focusWin = _viewWin;
	}
	else if (_focusWin == _viewWin && _rightPaneValid)
	{
		Win::UserMessage msg (UM_GIVEFOCUS);
		msg.SetLParam (_diffWin);
		_mainWin.SendMsg (msg);
		_focusWin = _diffWin;
	}		
}

//
// Helpers
//

bool Commander::NeedsSave () const
{
	Win::Dow::Handle win = _editorPool.GetEditableWindow ();
	if (win.IsNull ())
		return false;
	unsigned int newSize = 0;
	Win::UserMessage msg (UM_EDIT_NEEDS_SAVE, 0, reinterpret_cast<long>(&newSize));
	win.SendMsg (msg);
	return msg.GetResult () != 0;
}

// Returns true when edit changes saved or there were no edit changes
bool Commander::CheckForEditChanges ()
{
	if (NeedsSave ())
	{
		Out::Answer answer = TheOutput.Prompt ("Do you want to save edit changes?");
		if (answer == Out::Yes)
		{
			return SaveFile ();
		}
		else if (answer == Out::Cancel)
		{
			return false;
		}
	}
	return true;
}

void Commander::WriteEditBufferToFile (Win::Dow::Handle editWin, std::string const & path, int newSize)
{
	auto_array<char> buf (newSize);
	memset (&buf [0], 'x', newSize);
	Win::UserMessage msg (UM_GET_BUF, 0, reinterpret_cast<long>(&buf [0]));
	editWin.SendMsg (msg);
	if (msg.GetResult () != newSize)
		throw Win::Exception ("Internal Error: Cannot fill save buffer");
	MemFileNew file (path, File::Size (newSize, 0));
	memcpy (file.GetBuf (), &buf [0], newSize);	
}

bool Commander::IsDualPaneDisplay ()
{
	return _fileNames.AsDiffer ();
}

void Commander::InitViewing (bool refresh)
{
	dbg << "--> Commander::Init" << std::endl;
	bool prevRightPaneValid = _rightPaneValid;
	_leftPaneValid = false;
	_rightPaneValid = false;
	if (_fileNames.AsViewer ())
	{
		if (_fileNames.HasPath (FileAfter))
			Open (_fileNames.GetPath (FileAfter), FileAfter);
		else if (_fileNames.HasPath (FileBefore) && !_fileNames.HasPath (FileCurrent))
			Open (_fileNames.GetPath (FileBefore), FileBefore);
		else
		{
			std::string const & path = _fileNames.GetPath (FileCurrent);
			if (!path.empty ())
				Open (path, FileCurrent);
			else
				OpenNewFile ();
		}
	}
	else if (_fileNames.AsDiffer ())
	{
		if (!_fileNames.HasPath (FileBefore))
			throw Win::Exception ("Differ called without the path to the file before the change.");

		if (_fileNames.HasPath (FileAfter))
			Compare (_fileNames.GetPath (FileBefore), _fileNames.GetPath (FileAfter), EditStyle::chngSynch);
		else if (_fileNames.HasPath (FileCurrent))
			Compare (_fileNames.GetPath (FileBefore), _fileNames.GetPath (FileCurrent), EditStyle::chngUser);
		else
			throw Win::Exception ("Differ called without the path to the current file.");
	}
	else if (_fileNames.IsSingleFile ())
    {
		// Display single file
		if (_fileNames.HasPath (FileCurrent))
			Open (_fileNames.GetPath (FileCurrent), FileCurrent);
		else 
			OpenNewFile ();
    }

	if (!refresh)
	{
		// open new file
		Win::UserMessage msgBegin (UM_CLEAR_EDIT_STATE);
	    _viewWin.SendMsg (msgBegin);
	}

	if (refresh && _rightPaneValid != prevRightPaneValid)
	{
		// one pane -> two panes or two panes -> one pane for the same file
		Win::UserMessage msg (UM_PREPARE_CHANGE_NUMBER_PANES);
		_mainWin.PostMsg (msg);
	}
	// Notify mainWin which panes are valid
	int code = 0;
	if (!_leftPaneValid)
	{
		code = 1;
	}
	else if (!_rightPaneValid)
	{
		code = 2;
	}

	// _fileNames.Dump ();
	InitTabs ();
	Win::UserMessage msg (UM_HIDE_WINDOW, code);
	_mainWin.PostMsg (msg);
	dbg << "<-- Commander::Init" << std::endl;
}

void Commander::InitLeftPane ()
{
	bool noEdits = true;
	std::string path;
	FileSelection sel;
	if (_fileNames.HasPath (FileAfter))
	{
		sel = FileAfter;
		path = _fileNames.GetPath (FileAfter);
	}
	else if (_fileNames.HasPath (FileCurrent))
	{
		sel = FileCurrent;
		path = _fileNames.Project ();
		noEdits = !_fileNames.ProjectIsPresent ();
	}
	else
		throw Win::Exception ("Differ called with incorrect arguments");

	MemFileReadOnly file (path);
	File::Size fileSize = file.GetSize ();
	if (fileSize.IsLarge ())
		throw Win::Exception ("File size exceeds 4GB", 0, 0);
	LineBuf lineBuf (file.GetBuf (), fileSize.Low ());
	LineCounter counter;
	InitVersionDocument (lineBuf, counter, sel);
	InitHiddenDocuments (sel);
}
#if 0
void Commander::InitLeftPane (std::vector<char> const & bufMerge, Progress::Meter & meter)
{
	if (_fileNames.ProjectIsPresent ())
	{
		MemFileReadOnly file (_fileNames.GetPath (FileCurrent));
		File::Size fileSize = file.GetSize ();
		if (fileSize.IsLarge ())
			throw Win::Exception ("File size exceeds 4GB", _fileNames.Project ().c_str (), 0);

		FuzzyComparator comp;
		Differ diff (&bufMerge[0], bufMerge.size (), 
					 file.GetBuf (), fileSize.Low (), 
                     comp, meter, EditStyle::chngUser);
		LineCounterOrig counter;
		InitVersionDocument (diff, counter, FileCurrent);
	}
	else if (_fileNames.HasPath (FileCurrent))
	{
		// create empty document
		std::unique_ptr<EditBuf> editBuf (new EditBuf);		
		Win::UserMessage msg (UM_INITDOC);
		msg.SetLParam (&editBuf);
		_viewWin.SendMsg (msg);
		_leftPaneValid = true;
	}
	InitHiddenDocuments (FileCurrent);
}
#endif
void Commander::Open (std::string const & path, FileSelection sel)
{
	dbg << "--> Commander::Open" << std::endl;
	dbg << path << std::endl;
    try
    {
        MemFileReadOnly file (path);
		File::Size fileSize = file.GetSize ();
		if (fileSize.IsLarge ())
			throw Win::Exception ("File size exceeds 4GB", path.c_str (), 0);
		LineBuf lineBuf (file.GetBuf (), fileSize.Low ());
		LineCounter counter;
		InitVersionDocument (lineBuf, counter, sel);
		InitHiddenDocuments (sel);
    }
    catch (Win::Exception e)
    {
        if (e.GetError () == ERROR_SHARING_VIOLATION)
            TheOutput.Display ("The file is being used by another process", Out::Error);
		else
			throw;
    }
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Unexpected error: Opening a file", Out::Error);
	}
	dbg << "<-- Commander::Open" << std::endl;
}

void Commander::InitHiddenDocuments (FileSelection selExcept)
{
	for (int i = 0; i < FileMaxCount; ++i)
	{
		FileSelection sel = static_cast<FileSelection> (i);
		if (sel != selExcept && _fileNames.HasPath (sel))
		{
			std::string const & path = _fileNames.GetPath (sel);
			MemFileReadOnly file (path);
			File::Size fileSize = file.GetSize ();
			if (fileSize.IsLarge ())
				throw Win::Exception ("File size exceeds 4GB", path.c_str (), 0);
			LineBuf lineBuf (file.GetBuf (), fileSize.Low ());
			LineCounter counter;
			std::unique_ptr<EditBuf> editBuf (new EditBufTarget);
			Progress::Meter progress;
			lineBuf.Dump (*editBuf, counter, progress);
			_editorPool.AddDocument (sel, std::move(editBuf));
		}
	}
	if (_fileNames.HasPath (FileCurrent))
	{
		_editorPool.SetEditableReadOnly (File::IsReadOnly (_fileNames.GetPath (FileCurrent)));
	}
}

void Commander::Compare (std::string const & pathOld, std::string const & pathNew, EditStyle::Source src)
{
	dbg << "--> Commander::Compare" << std::endl;
	dbg << "    Old path: " << pathOld << std::endl;
	dbg << "    New path: " << pathNew << std::endl;
	if (!File::Exists (pathNew))
		throw Win::InternalException ("New file doesn't exist any more", pathNew.c_str ());
	if (!File::Exists (pathOld))
		throw Win::InternalException ("Old file doesn't exist any more", pathOld.c_str ());

    try
    {		
		auto_array<char> newBuf;
		int newBufSize = 0;
		auto_array<char> oldBuf;
		int oldBufSize = 0;			
		{	// scope of open file
			MemFileReadOnly newFile (pathNew);
			File::Size fileSize = newFile.GetSize ();
			if (fileSize.IsLarge ())
				throw Win::Exception ("File size exceeds 4GB", pathNew.c_str (), 0);
			newBufSize = fileSize.Low ();
			auto_array<char> buf (newBufSize);
			memcpy (& buf [0], newFile.GetBuf (), newBufSize);
			newBuf = buf;
		}
		{
			// scope of open file
			MemFileReadOnly oldFile (pathOld);
			File::Size fileSize = oldFile.GetSize ();
			if (fileSize.IsLarge ())
				throw Win::Exception ("File size exceeds 4GB", pathOld.c_str (), 0);
			oldBufSize = fileSize.Low ();
			auto_array<char> buf (oldBufSize);
			memcpy (& buf [0], oldFile.GetBuf (), oldBufSize);
			oldBuf = buf;
		}

		if (oldBufSize > Differ::MaxFileSize || newBufSize > Differ::MaxFileSize)
			throw Win::Exception ("Files are too big to compare in reasonable time");
		Compare (&oldBuf [0], oldBufSize, &newBuf [0], newBufSize, src);
    }
    catch (Win::Exception e)
    {
        if (e.GetError () == ERROR_SHARING_VIOLATION)
            TheOutput.Display ("The file is being used by another process");
		else
			throw;
    }
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Unexpected error: Comparing files", Out::Error);
	}
	dbg << "<-- Commander::Compare" << std::endl;
}

void Commander::Compare (std::string const & pathOld, char const * newBuf, int newBufSize, EditStyle::Source src)
{
	dbg << "--> Commander::Compare2" << std::endl;
	dbg << "    Old path: " << pathOld << std::endl;
	if (!File::Exists (pathOld))
		throw Win::InternalException ("Old file doesn't exist any more", pathOld.c_str ());
    try
    {		
		auto_array<char> oldBuf;
		int oldBufSize = 0;			
		{
			// scope of open file
			MemFileReadOnly oldFile (pathOld);
			File::Size fileSize = oldFile.GetSize ();
			if (fileSize.IsLarge ())
				throw Win::Exception ("File size exceeds 4GB", pathOld.c_str (), 0);
			oldBufSize = fileSize.Low ();
			auto_array<char> buf (oldBufSize);
			memcpy (& buf [0], oldFile.GetBuf (), oldBufSize);
			oldBuf = buf;
		}
		if (oldBufSize > Differ::MaxFileSize || newBufSize > Differ::MaxFileSize)
			throw Win::Exception ("Files are too big to compare in reasonable time");

		Compare (& oldBuf [0], oldBufSize, newBuf, newBufSize, src, true); // true keepLeftPane
    }
    catch (Win::Exception e)
    {
        if (e.GetError () == ERROR_SHARING_VIOLATION)
            TheOutput.Display ("The file is being used by another process");
		else
			throw;
    }
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Unexpected error: Comparing files", Out::Error);
	}
	dbg << "<-- Commander::Compare2" << std::endl;
}

void Commander::Compare (char const * oldBuf, int oldBufSize, char const * newBuf, int newBufSize, EditStyle::Source src, bool keepLeftPane)
{
	dbg << "--> Commander::Compare3" << std::endl;
	Progress::MultiMeterDialog progressDialog ("Code Co-op Differ", _mainWin, _msgPrepro);
	progressDialog.SetCaption ("Comparing files.");
	Progress::Meter & overall = progressDialog.GetOverallMeter ();
	Progress::Meter & specific = progressDialog.GetSpecificMeter ();

	overall.SetRange (0, 3);
	overall.SetActivity ("Analyzing files.");
	overall.StepAndCheck ();

	FuzzyComparator comp;
	Differ diff (oldBuf, oldBufSize, newBuf, newBufSize, comp, specific, src);
	overall.SetActivity ("Preparing right pane display");
	overall.StepAndCheck ();

	LineCounterDiff counter;
	InitDiffDocument (diff, counter, specific);

	bool filesContentEqual = false;
	if (oldBufSize == newBufSize)
		filesContentEqual = memcmp (oldBuf, newBuf, oldBufSize) == 0; 
	
	if (!filesContentEqual || keepLeftPane)
		_rightPaneValid = true;
	overall.SetActivity ("Preparing left pane display");
	overall.StepAndCheck ();
	if (!keepLeftPane)
		InitLeftPane ();
	else
		_leftPaneValid = true;
	dbg << "<-- Commander::Compare3" << std::endl;
}

void Commander::InitVersionDocument (LineDumper & dumper, LineCounter & counter, FileSelection sel)
{
	std::unique_ptr<EditBuf> editBuf (new EditBufTarget);
	Progress::Meter progress;
	dumper.Dump (*editBuf, counter, progress);
	_editorPool.InitDocument (sel, std::move(editBuf));
	_leftPaneValid = true;
}

void Commander::InitDiffDocument (LineDumper & dumper, LineCounter & counter, Progress::Meter & progress)
{
	std::unique_ptr<EditBuf> editBuf (new EditBuf);
	dumper.Dump (*editBuf, counter, progress);
	progress.StepAndCheck ();
	Win::UserMessage msg (UM_INITDOC);
	msg.SetLParam (&editBuf);
	_diffWin.SendMsg (msg);
	_rightPaneValid = true;
}

void Commander::OpenNewFile ()
{
	// create empty document
	std::unique_ptr<EditBuf> editBuf (new EditBuf);		
	_editorPool.InitDocument (FileCurrent, std::move(editBuf));	
	// set caption
	SetTitle(_fileNames.GetPath (FileCurrent));

	// Set panes and notify top window
	_leftPaneValid = true;
	_rightPaneValid = false;
	Win::UserMessage msg1 (UM_HIDE_WINDOW, 2);
	_mainWin.PostMsg (msg1);
	InitTabs ();
}

bool Commander::SaveFile ()
{
	Win::Dow::Handle win = _editorPool.GetEditableWindow ();
	if (win.IsNull ())
		return false;

	int newSize = 0;
	Win::UserMessage msg (UM_EDIT_NEEDS_SAVE, 0, reinterpret_cast<long>(&newSize));
	win.SendMsg (msg);
	if (msg.GetResult () != 0)
	{
		// Edit window needs save
		try
		{
			if (_fileNames.IsCheckedoutFile ())
			{
				// Save controlled file
				if (!File::Exists (_fileNames.Project ()))
					throw Win::InternalException ("Target file doesn't exist any more", _fileNames.Project ().c_str ());
				BackupFile backupFile (_fileNames.Project ());
				WriteEditBufferToFile (win, _fileNames.Project (), newSize);
				backupFile.Commit ();
				_fileNames.GetProjectFileTimeStamp ();
				//_differInfo.SetLocalEdit (true);
			}
			else
			{
				// Save not controlled file
				if (!_fileNames.HasPath (FileCurrent) || File::IsReadOnly (_fileNames.Project ()))
				{
					// No file path or under current file name file has read-only attribute
					// Ask for the target file name
					FileGetter  fileDlg;
					CurrentFolder curFolder;
					fileDlg.SetInitDir (curFolder.GetDir ());
					if (fileDlg.GetNewFile (_mainWin, "Save As"))
					{
						_fileNames.SetProjectFile (fileDlg.GetPath ());
						SetTitle(_fileNames.Project ());
						_fileNames.VerifyPaths ();
					}
					else
					{
						return false;
					}
				}
				WriteEditBufferToFile (win, _fileNames.Project (), newSize);
				_fileNames.GetProjectFileTimeStamp ();
			}
		}
		catch (Win::Exception e)
		{
			TheOutput.Display (e);
			return false;
		}
		catch (...)
		{
			Win::ClearError ();
			TheOutput.Display ("Internal Error: Cannot save edit changes", Out::Error);
			return false;
		}
	}
	return true;
}

void Commander::SetTitle(std::string const & path)
{
	if (path.empty())
		_mainWin.SetText ("Untitled");
	else
	{
		PathSplitter splitter (path);
		std::string title = splitter.GetFileName();
		title += splitter.GetExtension();
		title += "    ";
		title += splitter.GetDrive();
		title += splitter.GetDir();
		_mainWin.SetText (title);
	}
}

bool Commander::IsFocusOnEditable () const
{
	Win::Dow::Handle winEdit = _editorPool.GetEditableWindow ();
	return _focusWin == winEdit;
}

void Commander::InitTabs ()
{
	_tabs->Clear ();
	for (int i = 0; i < FileMaxCount; ++i)
	{
		FileSelection sel = static_cast<FileSelection> (i);
		if (_fileNames.HasPath (sel))
		{
			_tabs->InsertPageTab (sel, FileSelectionName [i]);
		}
	}
	if (_fileNames.HasPath (FileAfter))
		_tabs->SetSelection (FileAfter);
	else if (_fileNames.HasPath (FileCurrent))
		_tabs->SetSelection (FileCurrent);
}
