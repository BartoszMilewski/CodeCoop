//-----------------------------------------
// (c) Reliable Software 1997, 98, 99, 2000
//-----------------------------------------

#include "precompiled.h"
#include "MenuTable.h"
#include <Ctrl/Menu.h>

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item programItems [] =
	{
		{CMD,		"&Save and Exit",		"Program_ExitSave", 0},	// Exit and Save
		{CMD,		"&Exit",				"Program_Exit", 0},		// Exit
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&About",				"Program_About", 0},		// About
		{END,		0,						0, 0}
	};

	const Item fileItems [] =
	{
		{CMD,		"&Open ...",			"File_Open", 0},			// Open
		{CMD,		"&New\tCtrl+N",			"File_New", 0},			// New file
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Save\tCtrl+S",		"File_Save", 0},			// Save
		{CMD,		"Save &as ...",			"File_SaveAs", 0},			// Save as
		{CMD,		"&Refresh\tCtrl+R",		"File_Refresh", 0},		// Refresh
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Check-Out\tCtrl+O",	"File_CheckOut", 0},		// Check-Out
		{CMD,		"&Uncheck-Out\tCtrl+U",	"File_UncheckOut", 0},		// Uncheck-Out
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"Compare with...",		"File_CompareWith", 0},	// Compare cuurent file with other
		{END,		0,						0, 0}
	};

	const Item editItems [] =
	{
		{CMD,		"&Undo\tCtrl+Z",		"Edit_Undo", 0},			// Undo
		{CMD,		"&Redo\tCtrl+Y",		"Edit_Redo", 0},			// Redo
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Copy\tCtrl+C",		"Edit_Copy", 0},			// Copy
		{CMD,		"C&ut\tCtrl+X",			"Edit_Cut", 0},			// Cut
		{CMD,		"&Paste\tCtrl+V",		"Edit_Paste", 0},			// Paste
		{CMD,		"&Delete\tDel",			"Edit_Delete", 0},			// Delete
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"Select &All\tCtrl+A",	"Edit_SelectAll", 0},		// Select All
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"Change &Font ...",		"Edit_ChangeFont", 0},		// Change Font
		{CMD,		"Change &Tab Size ...",	"Edit_ChangeTab", 0},		// Change Tab size
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Word Wrap",			"Edit_ToggleLineBreaking", 0},// Toggle lines breaking
		{END,		0,						0, 0}
	};

	const Item searchItems [] =
	{
		{CMD,		"&Next Change\tF4",		"Search_NextChange", 0},	// Next Change
		{CMD,		"&Prev Change\tShift+F4","Search_PrevChange", 0},	// Prev Change
		{SEPARATOR,	0,						0, 0},						// ----------
		{CMD,		"&Find ...\tCtrl+F",	"Search_FindText", 0},     // Search dialog
		{CMD,		"Find &Next\tF3",       "Search_FindNext", 0},	    // Search next		
		{CMD,       "&Replace ...\tCtrl+H", "Search_Replace", 0},		//Replace dialog
		{CMD,		"&Go To Line...\tCtrl+G", "Search_GoToLine", 0},	//Go to line dialog
		{END,		0,						0, 0}
	};

	const Item helpItems [] =
	{
		{CMD,       "&Contents\tF1",        "Help_Contents", 0},		// Contents
		{END,       0,                     0, 0}
	};

	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "&Program",	"Program",	programItems},		// Program
		{POP,   "&File",	"File",		fileItems},			// File
		{POP,   "&Edit",	"Edit",		editItems},			// Edit
		{POP,   "&Search",	"Search",	searchItems},		// Search
		{POP,   "&Help",	"Help",		helpItems},			// Help
		{END,	0,			0,			0}
	};
};
