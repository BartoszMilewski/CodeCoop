//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"
#include "MenuTable.h"

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item programItems [] =
	{
		//{CMD,		"&Preferences",			"Program_Preferences", 0},	// Preferences
		{CMD,		"&Options...",			"Program_Options", 0},		// Options
		{CMD,		"Co-op &Update...",	    "Program_Update", 0},		// Check for updates on the Internet
		{CMD,		"Configure &Dispatching...", "Program_Dispatching", 0},	// Open Dispatcher configuration wizard
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&License...",		    "Program_Licensing", 0},	// Licensing
		{CMD,		"&About...",			"Program_About", 0},		// About
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"E&xit",				"Program_Exit", 0},			// Exit
		{END,		0,						0, 0}
	};

	const Item projectItems [] =
	{
		{CMD,		"&Visit...",			"Project_Visit", 0},		// Visit
		{CMD,		"&Repair",				"Project_Repair", 0},		// Repair
		{CMD,		"Re&quest Verification...", "Project_RequestVerification", 0},	// Request Verification
		{CMD,		"Verify Root &Paths...", "Project_VerifyRootPaths", 0},	// Verify Root Paths
		{CMD,		"Add More &Files...",	"Project_AddMoreFiles", 0},	// Add More Files
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Join...",				"Project_Join", 0},			// Join
		{CMD,		"&Invite...",			"Project_Invite", 0},		// Invite
		{CMD,		"&Open Invitation...",	"Project_OpenInvitation", 0},		// Download Invitation
		{CMD,		"&New...",				"Project_New", 0},			// New
		{CMD,		"New From &History...",	"Project_NewFromHistory", 0}, // New From History
		{CMD,		"&Branch...",			"Project_Branch", 0},		// Branch
		{CMD,		"&Merge With Branch Project...",	"View_MergeProjects", 0},	// Project merge
		{CMD,		"&Defect (Delete)...",	"Project_Defect", 0},		// Defect
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"M&ove...",				"Project_Move", 0},			// Move
		{CMD,		"&Export...",			"Project_Export", 0},		// Export
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"Membe&rs...",			"Project_Members", 0},		// Members
		{CMD,		"&Administrator...",	"Project_Admin", 0},		// Administrator
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"Op&tions...",			"Project_Options", 0},		// Options
		{END,		0,						0, 0}
	};

	const Item historyItems [] =
	{
								// -------
		{CMD,		"&Export History...",	"History_Export", 0},		// Export History
		{CMD,		"&Import History...",	"History_Import", 0},		// Import History
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Add Label...",		"History_AddLabel", 0},		// Label
		{END,		0,						0, 0}
	};

	const Item viewItems [] =
	{
		{CMD,		"Bac&k",				"View_Back", 0},			// Browse back
		{CMD,		"For&ward",				"View_Forward", 0},			// Browse forward
		{CMD,		"Ho&me",				"View_Home", 0},			// Browse home
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Apply File Filter",	"View_ApplyFileFilter", 0},	// Apply File Filter
		{CMD,		"C&hange File Filter...","View_ChangeFileFilter", 0},// Change File Filter
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Folder Tree",			"View_Project_Folders", 0},			// Project Folders
		{CMD,		"A&ctive Projects",		"View_Active_Projects", 0},			// Active Projects
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Browser",				"View_Browser", 0},			// Web Browser
		{CMD,		"&Files",				"View_Files", 0},			// Files
		{CMD,		"&Check-In Area",		"View_CheckIn", 0},			// Check-In Area
		{CMD,		"&Inbox",				"View_Mailbox", 0},			// Mailbox
		{CMD,		"&History",				"View_History", 0},			// History
		{CMD,		"&Projects",			"View_Projects", 0},		// Projects
		{CMD,		"&Sync Merge",			"View_Synch", 0},			// Sync Merge
		{CMD,		"Branch &Merge",		"View_MergeProjects", 0},	// Branch Merge
		{SEPARATOR,	0,						0, 0},						// -------
		{CMD,		"&Refresh\tF5",			"View_Refresh", 0},			// Refresh
		{CMD,		"Close &View",			"View_ClosePage", 0},		// Close View
		{END,		0,						0, 0}
	};

	const Item folderItems [] =
	{
		{CMD,		"&Go Up\tBksp",			"Folder_GoUp", 0},			// Go Up
		{CMD,		"Goto &Root",			"Folder_GotoRoot", 0},		// Goto Root
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,		"&New File...",			"Folder_NewFile", 0},		// New File ...
		{CMD,		"New &Folder...",		"Folder_NewFolder", 0},		// New Folder
		{CMD,		"&Add More Files...",	"Folder_AddMoreFiles", 0},	// Add More Files
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,		"&Wikify",				"Folder_Wikify", 0},	    // Wikify
		{CMD,		"&Export Wiki as HTML...",	"Folder_ExportHtml", 0},	// Export as HTML
		{END,		0,						0, 0}
	};

	const Item allItems [] =
	{
		{CMD,		"Check-&In...\tCtrl+I",	"All_CheckIn", 0},			// Check-In
		{CMD,		"Check-&Out",			"All_CheckOut", 0},			// Check-Out
		{CMD,		"&Deep Check-Out",		"All_DeepCheckOut", 0},		// Deep Check-Out
		{CMD,		"&Un-CheckOut",		    "All_Uncheckout", 0},		// Un-CheckOut
		{CMD,		"Save &File Version",	"All_SaveFileVersion", 0},	// Save File Version
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,		"&Sync Scripts\tCtrl+S","All_Synch", 0},			// Synch
		{CMD,		"Accept S&ync\tCtrl+Y",	"All_AcceptSynch", 0},		// Accept Synch
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,		"&Report...",			"All_Report", 0},			// Report
		{END,		0,						0, 0}
	};

	const Item selectionItems [] =
	{
		// Begin of File/Check-in Area selection
		{CMD,		"Check-&Out\tCtrl+O",	"Selection_CheckOut", 0},		// Check-Out
		{CMD,		"&Deep Check-Out",		"Selection_DeepCheckOut", 0},	// Deep Check-Out
		{CMD,		"Check-&In...",			"Selection_CheckIn", 0},		// Check-In
		{CMD,		"&Un-CheckOut",		    "Selection_Uncheckout", 0},		// Un-CheckOut
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"&Add To Project",		"Selection_Add", 0},			// Add To Project
		{CMD,		"&Rename...\tF2",		"Selection_Rename", 0},			// Rename
		{CMD,		"Move...",				"Selection_Move", 0},
		{CMD,		"&Delete\tDel",			"Selection_Delete", 0},			// Delete
		{CMD,		"&Make Uncontrolled",	"Selection_Remove", 0},			// Remove From Project 
		{CMD,		"Change File &Type...",	"Selection_ChangeFileType", 0},	//Change File Type
		{CMD,		"Add &Extension to File Filter","Selection_Add2FileFilter", 0},// Hide Files With This Extension
		{CMD,		"Remo&ve Extension from File Filter","Selection_RemoveFromFileFilter", 0},// Show Files With This Extension
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"&Cut\tCtrl+X",			"Selection_Cut", 0},			// Cut
		{CMD,		"Cop&y\tCtrl+C",		"Selection_Copy", 0},			// Copy
		{CMD,		"&Paste\tCtrl+V",		"Selection_Paste", 0},			// Paste
		// Begin of Mailbox/Synch Area selection
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"&Sync Next Script\tCtrl+N","Selection_Synch", 0},		// Synch
		{CMD,		"Re&quest Script Resend",	"Selection_RequestResend", 0},  // Request Resend
		{CMD,		"&Edit Sync",			"Selection_EditSync",	0},		// Edit Sync
		{CMD,		"Accept S&ync",			"Selection_AcceptSynch", 0},	// Accept Synch
		// Begin of History selection
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"Show Changes M&ade By This Script...","Selection_CompareWithPrevious", 0},		// Compare With Previous
		{CMD,		"Sho&w Changes Since This Script...",	"Selection_CompareWithCurrent", 0},		// Compare With Current
		{CMD,		"Co&mpare Script Changes...",			"Selection_Compare", 0},// Compare
		{SEPARATOR,	0,						0, 0},							// -------
		{CMD,		"Res&tore...",			"Selection_Revert", 0},			// Restore
		{CMD,		"&Merge With Current Version...",	"Selection_MergeBranch", 0},	// Merge Local Branch
		{CMD,		"Merge With Branch &Project...",	"View_MergeProjects", 0},	// Project merge
		{CMD,		"&Branch...",			"Selection_Branch", 0},			// Branch
		{CMD,		"E&xport Version...",	"Selection_VersionExport", 0},	// Export Version
		{CMD,		"A&rchive",				"Selection_Archive", 0},		// Archive
		{CMD,		"U&nArchive",			"Selection_UnArchive", 0},		// UnArchive
		// Begin of File/History common part
		{SEPARATOR,	0,						0},
		{CMD,		"Show &History",		"Selection_ShowHistory", 0},	// Show History
		{CMD,		"&Blame",				"Selection_Blame",		0},		// Blame
		{CMD,		"R&e-send Script(s)...","Selection_SendScript", 0},		// Send Script
		// Begin of File/History/Mailbox/Synch Area common part
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"O&pen\tEnter",			"Selection_Open", 0},			// Open
		{CMD,		"Open with She&ll\tCtrl+Enter", "Selection_OpenWithShell", 0},	// Open wit Shell
		{CMD,		"Sea&rch with Shell",	"Selection_SearchWithShell", 0},// Search wit Shell
		{CMD,		"R&eport...",			"Selection_Report", 0},			// Report
		{CMD,		"Copy Scr&ipt Comment",	"Selection_CopyScriptComment", 0},	// Copy Script Comment
		{CMD,		"Save &File Version",	"Selection_SaveFileVersion", 0},// Save File Version
		{CMD,		"Restore File &Version","Selection_RestoreFileVersion", 0},// Restore File Version
		{CMD,		"&Merge File Version",	"Selection_MergeFileVersion", 0},// Merge File Version
		{CMD,		"&Auto Merge File Version",	"Selection_AutoMergeFileVersion", 0},// Merge File Version
		{CMD,		"&Reload",				"Selection_Reload", 0},
		{SEPARATOR,	0,						0, 0},							// --------
		{CMD,		"&Properties\tAlt+Enter","Selection_Properties", 0},
		{END,		0,						0, 0}
	};

	const Item toolsItems [] =
	{
		{CMD,       "&Differ...",			"Tools_Differ", 0},
		{CMD,       "&Merger...",			"Tools_Merger", 0},
		{CMD,       "&Editor...",			"Tools_Editor", 0},
		{SEPARATOR,	0,						0, 0},
		{CMD,		"&Create Backup...",	"Tools_CreateBackup", 0},
		{CMD,		"&Restore From Backup...", "Tools_RestoreFromBackup", 0},
		{SEPARATOR,	0,						0, 0},
		{CMD,		"Move to &New Machine...", "Tools_MoveToMachine", 0},
		{END,       0,						0, 0}
	};

	const Item helpItems [] =
	{
		{CMD,       "&Contents\tF1",		"Help_Contents", 0},		// Content
		{CMD,       "&Index",				"Help_Index", 0},			// Index
		{CMD,       "&Tutorial",			"Help_Tutorial", 0},		// Tutorial
		{CMD,		"&Support",				"Help_Support", 0},		// Support
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,		"Save &Diagnostics...",	"Help_SaveDiagnostics", 0},// Save Diagnostics
		{CMD,		"Restore &Original Installation", "Help_RestoreOriginal", 0}, // Restore Original
		{SEPARATOR,	0,						0, 0},						// --------
		{CMD,       "&Beginner Mode",		"Help_BeginnerMode", 0},	// Beginner Mode
		{END,       0,                     0, 0}
	};

	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "P&rogram",		"Program",	programItems},		//Program
		{POP,   "&Project",		"Project",	projectItems},		//Project
		{POP,   "H&istory",		"History",	historyItems},		//History
		{POP,   "Vie&w",		"View",		viewItems},			//View
		{POP,   "&Folder",		"Folder",	folderItems},		//Folder
		{POP,   "&All",			"All",		allItems},			//All
		{POP,   "&Selection",	"Selection",selectionItems},	//Selection
		{POP,   "&Tools",		"Tools",	toolsItems},		//Tools
		{POP,   "&Help",		"Help",		helpItems},			//Help
		{END,   0,				0,			0}
	};
};
