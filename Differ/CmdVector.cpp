//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

namespace Cmd
{
	// Differ Commands

	Cmd::Item<Commander> const Table [] =
	{
																											
		{ "Program_ExitSave",		&Commander::Program_ExitSave,		&Commander::can_Program_ExitSave,    "Save changes and exit" },
		{ "Program_Exit",			&Commander::Program_Exit,			&Commander::can_Program_Exit,		 "Exit program" },
		{ "Program_About",			&Commander::Program_About,			0,       "About this program" },

		
		{ "File_Open",				&Commander::File_Open,				&Commander::can_File_Open,           "Open a file" },
		{ "File_New",				&Commander::File_New,				&Commander::can_File_New,            "Open a new file" },
		{ "File_Save",				&Commander::File_Save,				&Commander::can_File_Save,			 "Save changes" },
		{ "File_SaveAs",			&Commander::File_SaveAs,			&Commander::can_File_SaveAs,		 "Save changes as ..." },
		{ "File_Refresh",			&Commander::File_Refresh,			&Commander::can_File_Refresh,        "Save changes and re-display file" },
		{ "File_CheckOut",			&Commander::File_CheckOut,			&Commander::can_File_CheckOut,		 "Prepare file for editing" },
		{ "File_UncheckOut",		&Commander::File_UncheckOut,		&Commander::can_File_UncheckOut,	 "Restore to original file version" },
		{ "File_CompareWith",		&Commander::File_CompareWith,		&Commander::can_File_CompareWith,	 "Compare cuurent file with other" },

																											
		{ "Edit_Undo",				&Commander::Edit_Undo,				&Commander::can_Edit_Undo,           "Undo last edit action" },
		{ "Edit_Redo",				&Commander::Edit_Redo,				&Commander::can_Edit_Redo,           "Redo last undo action" },
		{ "Edit_Copy",				&Commander::Edit_Copy,				&Commander::can_Edit_Copy,           "Copy selection to clipboard" },
		{ "Edit_Cut",				&Commander::Edit_Cut,				&Commander::can_Edit_Cut,            "Cut selection into clipboard" },
		{ "Edit_Paste",				&Commander::Edit_Paste,				&Commander::can_Edit_Paste,          "Paste text from clipboard" },
		{ "Edit_Delete",			&Commander::Edit_Delete,			&Commander::can_Edit_Delete,         "Delete selection" },
		{ "Edit_SelectAll",			&Commander::Edit_SelectAll,			&Commander::can_Edit_SelectAll,		 "Select all" },
		{ "Edit_ChangeFont",		&Commander::Edit_ChangeFont,		&Commander::can_Edit_ChangeFont,     "Pick new font" },
		{ "Edit_ChangeTab",			&Commander::Edit_ChangeTab,			0,									 "Change tab/indent size" },
		{ "Edit_ToggleLineBreaking",&Commander::Edit_ToggleLineBreaking,&Commander::can_Edit_ToggleLineBreaking, "Toggle line wrapping" },

																											
		{ "Search_NextChange",		&Commander::Search_NextChange,		&Commander::can_Search_NextChange,   "Go to next change" },
		{ "Search_PrevChange",		&Commander::Search_PrevChange,		&Commander::can_Search_PrevChange,   "Go to previous change" },
		{ "Search_FindText",	    &Commander::Search_FindText,	    &Commander::can_Search_FindText,     "Find text in file" },
		{ "Search_FindNext",		&Commander::Search_FindNext,		&Commander::can_Search_FindNext,     "Find next occurrence" },
		{ "Search_FindPrev",		&Commander::Search_FindPrev,		&Commander::can_Search_FindNext,     "Find previous occurrence" },
		{ "Search_Replace",         &Commander::Search_Replace,         &Commander::can_Search_Replace,      "Replace text in file" },
		{ "Search_GoToLine",        &Commander::Search_GoToLine,        &Commander::can_Search_GoToLine,     "Go to selected line in text"},

																											
		{ "Help_Contents",			&Commander::Help_Contents,			&Commander::can_Help_Contents,       "Display help" },

		{ "Nav_NextPanel",			&Commander::Nav_NextPanel,			0,									 0 },
		{ 0, 0, 0, 0 }
	};
}

