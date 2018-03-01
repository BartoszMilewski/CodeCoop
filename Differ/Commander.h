#if !defined (COMMANDER_H)
#define COMMANDER_H
//----------------------------------
// (c) Reliable Software 1998 - 2007
//----------------------------------

#include "FileNames.h"
#include "EditStyle.h"
#include "FindDlg.h"
#include "TabCtrl.h"

#include <Ctrl/Command.h>
#include <Win/Win.h>

namespace Win { class MessagePrepro; }
namespace Progress { class Meter; }
class LineDumper;
class LineCounter;
class EditorPool;

class Commander
{
public:
	Commander (Win::Dow::Handle mainWnd,
			   Win::Dow::Handle & viewWin,
			   Win::Dow::Handle & focusWin,
			   EditorPool & editorPool,
			   FileNames & fileNames,
			   Win::MessagePrepro & msgPrepro);
	void AddTabView (FileTabController::View * tabs) { _tabs = tabs; }
	void AddFindPrompter (FindPrompter * findPrompter) { _findPrompter = findPrompter; }
    // Command execs and testers
												//Program
    void Program_ExitSave ();					//  Exit Save
	Cmd::Status can_Program_ExitSave () const { return Cmd::Enabled; }
	void Program_Exit ();						//  Exit Discard
	Cmd::Status can_Program_Exit () const { return Cmd::Enabled; }
    void Program_About ();						//  About

												//File
	void File_New ();							// New File
	Cmd::Status can_File_New () const;
    void File_Open ();							//  Open
	Cmd::Status can_File_Open () const;
    void File_Save ();							//  Save
	Cmd::Status can_File_Save () const;
	void File_SaveAs ();						// Sawe as...
	Cmd::Status can_File_SaveAs () const;
    void File_Refresh ();						//  Refresh
	Cmd::Status can_File_Refresh () const;
	void File_CheckOut ();						//  Check-Out
	Cmd::Status can_File_CheckOut () const;
	void File_UncheckOut ();					//  Uncheck-Out
	Cmd::Status can_File_UncheckOut () const;
	void File_Info ();							// Information about file
	Cmd::Status can_File_Info () const;
	void File_CompareWith ();
	Cmd::Status can_File_CompareWith () const;	// Compare cuurent file with other

												//Edit
    void Edit_Undo ();							//  Undo
	Cmd::Status can_Edit_Undo () const;
	void Edit_Redo ();
	Cmd::Status can_Edit_Redo () const;         // redo
    void Edit_Copy ();							//  Copy
	Cmd::Status can_Edit_Copy () const;
    void Edit_Cut ();							//  Cut
	Cmd::Status can_Edit_Cut () const;
    void Edit_Paste ();							//  Paste
	Cmd::Status can_Edit_Paste () const;
    void Edit_Delete ();						//  Delete
	Cmd::Status can_Edit_Delete () const;
    void Edit_SelectAll ();						//  Select All
	Cmd::Status can_Edit_SelectAll () const;
    void Edit_ChangeFont ();					//  Change Font
	Cmd::Status can_Edit_ChangeFont () const;
	void Edit_ChangeTab ();						//  Change Tab
	void Edit_ToggleLineBreaking ();
	Cmd::Status can_Edit_ToggleLineBreaking () const;
												//Search
    void Search_NextChange ();					//  Next Change
	Cmd::Status can_Search_NextChange () const;
    void Search_PrevChange ();					//  Prev Change
	Cmd::Status can_Search_PrevChange () const;
	void Search_FindText ();                    // Open search dialog
	Cmd::Status can_Search_FindText () const;
	void Search_FindNext ();                    // Find next
	void Search_FindPrev ();
	Cmd::Status can_Search_FindNext () const;
	void Search_Replace ();                     // Open replace dialog
	Cmd::Status can_Search_Replace () const;
	void Search_GoToLine ();                    // Open "go to line" dialog
	Cmd::Status can_Search_GoToLine () const;

												//Help
    void Help_Contents ();						//  Content
	Cmd::Status can_Help_Contents () const { return Cmd::Enabled; }

	void Nav_NextPanel ();

	void InitViewing (bool refresh = false);
	bool IsDualPaneDisplay ();
	bool CheckForEditChanges ();

	void Open (std::string const & path, FileSelection sel);
	bool CanEdit () const { return IsFocusOnEditable (); }
	void SetTitle(std::string const & path);
private:
	bool IsFocusOnEditable () const;
	void InitTabs ();
	bool NeedsSave () const;
	void WriteEditBufferToFile (Win::Dow::Handle editWin, std::string const & path, int newSize);
	void InitLeftPane ();
//	void InitLeftPane (std::vector<char> const & bufMerge, Progress::Meter & meter);
    void Compare (std::string const & pathOld, std::string const & pathNew, EditStyle::Source src);
	void Compare (char const * oldBuf, int oldBufSize, char const * newBuf, int newBufSize, EditStyle::Source src, bool keepLeftPane = false);
	void Compare (std::string const & pathOld, char const * newBuf, int newBufSize, EditStyle::Source src);
	void Merge ();
	void OpenNewFile ();
	bool SaveFile (); // return true if the file was actually saved
	void InitVersionDocument (LineDumper & dumper, LineCounter & counter, FileSelection sel);
	void InitDiffDocument (LineDumper & dumper, LineCounter & counter, Progress::Meter & progress);
	void InitHiddenDocuments (FileSelection selExcept);
private:
	Win::Dow::Handle	_mainWin;
	Win::Dow::Handle &	_viewWin;
	Win::Dow::Handle	_diffWin;
	Win::Dow::Handle &	_focusWin;
	EditorPool &		_editorPool;
	FileNames &			_fileNames;
	FileTabController::View * _tabs;
	Win::MessagePrepro & _msgPrepro;
	bool				 _leftPaneValid;
	bool				 _rightPaneValid;
	FindPrompter       * _findPrompter;
};

#endif
