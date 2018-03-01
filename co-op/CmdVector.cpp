//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

// Code Co-op Commands

namespace Cmd
{
	const Cmd::Item<Commander> Table [] =
	{

		{ "Program_Update",			&Commander::Program_Update,			0,								    "Check for Co-op updates on the Internet" },
		{ "Program_Options",		&Commander::Program_Options,		0,									"View/Change program options" },
		{ "Program_Dispatching",	&Commander::Program_Dispatching,	0,									"View/Change script dispatching options" },
		{ "Program_Licensing",		&Commander::Program_Licensing,		0,									"Go to Web site to purchase license(s)" },
		{ "Program_About",			&Commander::Program_About,			0,									"About this program" },
		{ "Program_Exit",			&Commander::Program_Exit,			0,									"Exit program" },


		{ "Project_Visit",			&Commander::Project_Visit,			&Commander::can_Project_Visit,      "Visit another project on this machine" },
		{ "Project_Move",			&Commander::Project_Move,			&Commander::can_Project_Move,		"Move project tree to another location" },
		{ "Project_Repair",			&Commander::Project_Repair,			&Commander::can_Project_Repair,		"Repair project file(s)" },
		{ "Project_RequestVerification", &Commander::Project_RequestVerification, &Commander::can_Project_RequestVerification, "Request project verification data from some other project member" },
		{ "Project_VerifyRootPaths",&Commander::Project_VerifyRootPaths,0,									"Check if all projects have a valid root path" },
		{ "Project_Join",			&Commander::Project_Join,			0,									"Join an existing project" },
		{ "Project_Invite",			&Commander::Project_Invite,			&Commander::can_Project_Invite,     "Invite someone to join your project" },
		{ "Project_OpenInvitation",	&Commander::Project_OpenInvitation, 0,									"Download project invitation" },
		{ "Project_New",			&Commander::Project_New,			0,									"Start a new project" },
		{ "Project_Branch",			&Commander::Project_Branch,			&Commander::can_Project_Branch,		"Start a branch project" },
		{ "Project_Export",			&Commander::Project_Export,			&Commander::can_Project_Export,		"Export project files and folders" },
		{ "Project_Members",		&Commander::Project_Members,		&Commander::can_Project_Members,    "List/Edit project members' info" },
		{ "Project_Defect",			&Commander::Project_Defect,			&Commander::can_Project_Defect,     "Rescind your membership in this project" },
		{ "Project_Admin",			&Commander::Project_Admin,			&Commander::can_Project_Admin,		"Emergency project administrator election" },
		{ "Project_Options",		&Commander::Project_Options,		&Commander::can_Project_Options,	"View/Change project options" },
		{ "Project_AddMoreFiles",	&Commander::Project_AddMoreFiles,	&Commander::can_Project_AddMoreFiles,"Add more files to the project" },
		{ "Project_NewFromHistory",	&Commander::Project_NewFromHistory, &Commander::can_Project_NewFromHistory,"Start a new project from previously exported history" },


		{ "History_Export",			&Commander::History_ExportHistory,	&Commander::can_History_Export,		"Export project history" },
		{ "History_Import",			&Commander::History_ImportHistory,	&Commander::can_History_Import,		"Import project history" },
		{ "History_AddLabel",		&Commander::History_AddLabel,		&Commander::can_History_AddLabel,	"Label project version" },


		{ "View_Back",				&Commander::View_Back,				&Commander::can_View_Back,			"Navigate back" },
		{ "View_Forward",			&Commander::View_Forward,			&Commander::can_View_Forward,		"Navigate forward" },
		{ "View_Home",				&Commander::View_Home,				&Commander::can_View_Home,			"Navigate home" },
		{ "View_Refresh",			&Commander::View_Refresh,			&Commander::can_View_Refresh,		"Update displayed information" },
		{ "View_ApplyFileFilter",	&Commander::View_ApplyFileFilter,	&Commander::can_View_ApplyFileFilter,"Apply file filter" },
		{ "View_ChangeFileFilter",	&Commander::View_ChangeFileFilter,	&Commander::can_View_ChangeFileFilter,"Change file filter" },
		{ "View_Browser",			&Commander::View_Browser,			&Commander::can_View_Browser,       "View web page" },
		{ "View_Files",				&Commander::View_Files,				&Commander::can_View_Files,         "Browse through files in the project" },
		{ "View_Mailbox",			&Commander::View_Mailbox,			&Commander::can_View_Mailbox,       "Look at outgoing/incoming scripts" },
		{ "View_CheckIn",			&Commander::View_CheckIn,			&Commander::can_View_CheckIn,       "Look at checked-out files" },
		{ "View_Synch",				&Commander::View_Synch,				&Commander::can_View_Synch,         "Look at files changed by the sync" },
		{ "View_History",			&Commander::View_History,			&Commander::can_View_History,       "Look at the history of the project" },
		{ "View_Projects",			&Commander::View_Projects,			&Commander::can_View_Projects,      "Look at projects present on this machine" },
		{ "View_MergeProjects",		&Commander::View_MergeProjects,		&Commander::can_View_MergeProjects, "Open project merge view" },
		{ "View_Project_Folders",	&Commander::View_Hierarchy,			&Commander::can_View_Hierarchy,		"Show a hierarchical view panel for the current list" },
		{ "View_Active_Projects",	&Commander::View_Active_Projects,	&Commander::can_View_Active_Projects,"Show active projects (projects with checked out files or incoming scripts)" },
		{ "View_Next",				&Commander::View_Next,				0,									"" },
		{ "View_Previous",			&Commander::View_Previous,			0,									"" },
		{ "View_ClosePage",			&Commander::View_ClosePage,			&Commander::can_View_ClosePage,		"Close page" },
		{ "ChangeFilter",			&Commander::ChangeFilter,			0,									"" },
		{ "GoBack",					&Commander::GoBack,					0,									"" },


		{ "Folder_GoUp",			&Commander::Folder_GoUp,			&Commander::can_Folder_GoUp,        "Go to parent folder" },
		{ "Folder_GotoRoot",		&Commander::Folder_GotoRoot,		&Commander::can_Folder_GotoRoot,    "Go to project's root folder" },
		{ "Folder_NewFile",			&Commander::Folder_NewFile,			&Commander::can_Folder_NewFile,     "Create a new file in the project" },
		{ "Folder_NewFolder",		&Commander::Folder_NewFolder,		&Commander::can_Folder_NewFolder,	"Create a new folder in the project" },
		{ "Folder_AddMoreFiles",	&Commander::Folder_AddMoreFiles,	&Commander::can_Folder_AddMoreFiles,"Add more files to the project from the current folder" },
		{ "Folder_Wikify",			&Commander::Folder_Wikify,			&Commander::can_Folder_Wikify,		"Turn current folder into a wiki site" },
		{ "Folder_ExportHtml",		&Commander::Folder_ExportHtml,		&Commander::can_Folder_ExportHtml,	"Export wiki folder as HTML web site" },


		{ "All_CheckIn",			&Commander::All_CheckIn,			&Commander::can_All_CheckIn,		"Check-in all files" },
		{ "All_CheckOut",			&Commander::All_CheckOut,			&Commander::can_All_CheckOut,       "Check-out all files in current view" },
		{ "All_DeepCheckOut",		&Commander::All_DeepCheckOut,		&Commander::can_All_DeepCheckOut,   "Check-out all files including sub-folders" },
		{ "All_Uncheckout",			&Commander::All_Uncheckout,			&Commander::can_All_Uncheckout,     "Un-Checkout all checked-out files" },
		{ "All_AcceptSynch",		&Commander::All_AcceptSynch,		&Commander::can_All_AcceptSynch,    "Accept this sync in full" },
		{ "All_Synch",				&Commander::All_Synch,				&Commander::can_All_Synch, 			"Execute and accept all sync scripts" },
		{ "All_Report",				&Commander::All_Report,				&Commander::can_All_Report,         "Create report from all view items" },
		{ "All_Select",				&Commander::All_Select,				&Commander::can_All_Select,         "Select all view items" },
		{ "All_SaveFileVersion",	&Commander::All_SaveFileVersion,	&Commander::can_All_SaveFileVersion, "Save version of file(s)"		},


		{ "Selection_CheckOut",		&Commander::Selection_CheckOut,		&Commander::can_Selection_CheckOut, "Check-out selected files" },
		{ "Selection_DeepCheckOut",	&Commander::Selection_DeepCheckOut,&Commander::can_Selection_DeepCheckOut, "Check-out selected files including sub-folders" },
		{ "Selection_CheckIn",		&Commander::Selection_CheckIn,		&Commander::can_Selection_CheckIn,	"Check-in selected files" },
		{ "Selection_Uncheckout",	&Commander::Selection_Uncheckout,	&Commander::can_Selection_Uncheckout,"Un-Checkout selected files" },
		{ "Selection_Properties",	&Commander::Selection_Properties,	&Commander::can_Selection_Properties,"Selected item properties" },
		{ "Selection_Open",			&Commander::Selection_Open,			&Commander::can_Selection_Open,     "Open selected files for viewing/editing" },
		{ "Selection_OpenWithShell",&Commander::Selection_OpenWithShell,&Commander::can_Selection_OpenWithShell,"Open selected files for viewing/editing using Windows Shell" },
		{ "Selection_SearchWithShell",&Commander::Selection_SearchWithShell,&Commander::can_Selection_SearchWithShell,"Search selected folder using Windows Shell" },
		{ "Selection_Add",			&Commander::Selection_Add,			&Commander::can_Selection_Add,      "Add selected file(s) to project" },
		{ "Selection_Remove",		&Commander::Selection_Remove,		&Commander::can_Selection_Remove,   "Remove selected file(s) from project (don't delete!)" },
		{ "Selection_Delete",		&Commander::Selection_Delete,		&Commander::can_Selection_Delete,   "Delete selected file(s)" },
		{ "Selection_DeleteFile",	&Commander::Selection_DeleteFile,	0,	"" },
		{ "Selection_DeleteScript",	&Commander::Selection_DeleteScript,	0,	"" },
		{ "Selection_Rename",		&Commander::Selection_Rename,		&Commander::can_Selection_Rename,   "Rename selected file" },
		{ "Selection_Move",	        &Commander::Selection_MoveFiles,	&Commander::can_Selection_MoveFiles,"Move selected file(s)" },
		{ "Selection_ChangeFileType",&Commander::Selection_ChangeFileType,	&Commander::can_Selection_ChangeFileType,   "Change type" },
		{ "Selection_Cut",			&Commander::Selection_Cut,			&Commander::can_Selection_Cut,      "Cut selected file(s) (to be pasted into another folder)" },
		{ "Selection_Copy",			&Commander::Selection_Copy,			&Commander::can_Selection_Copy,     "Copy selected file(s) to Windows clipboard" },
		{ "Selection_Paste",		&Commander::Selection_Paste,		&Commander::can_Selection_Paste,    "Move file(s) that have been cut or copied to current folder" },
		{ "Selection_AcceptSynch",	&Commander::Selection_AcceptSynch,	&Commander::can_Selection_AcceptSynch,"Accept sync changes for selected file(s)" },
		{ "Selection_Synch",		&Commander::Selection_Synch,		&Commander::can_Selection_Synch,			"Execute and accept next sync script" },
		{ "Selection_EditSync",		&Commander::Selection_EditSync,		&Commander::can_Selection_EditSync,	"Execute next sync script and edit its result" },
		{ "Selection_SendScript",	&Commander::Selection_SendScript,	&Commander::can_Selection_SendScript,"Re-send script to project members" },
		{ "Selection_RequestResend",&Commander::Selection_RequestResend,&Commander::can_Selection_RequestResend,"Request script to be re-send to you" },
		{ "Selection_ShowHistory",	&Commander::Selection_ShowHistory,	&Commander::can_Selection_ShowHistory,"Show history of file(s)" },
		{ "Selection_Blame",		&Commander::Selection_Blame,		&Commander::can_Selection_Blame,	"Who did it?" },
		{ "Selection_Report",		&Commander::Selection_Report,		&Commander::can_Selection_Report,	"Create report from selected view item(s)" },
		{ "Selection_Compare",		&Commander::Selection_Compare,		&Commander::can_Selection_Compare,	"Compare old versions of file(s) " },
		{ "Selection_CompareWithPrevious",&Commander::Selection_CompareWithPrevious,	&Commander::can_Selection_CompareWithPrevious,	"Compare with previous version of file(s) " },
		{ "Selection_CompareWithCurrent",&Commander::Selection_CompareWithCurrent,	&Commander::can_Selection_CompareWithCurrent,	"Compare with current version of file(s) " },
		{ "Selection_Revert",		&Commander::Selection_Revert,		&Commander::can_Selection_Revert,		"Restore old version of file(s)" },
		{ "Selection_MergeBranch",	&Commander::Selection_MergeBranch,	&Commander::can_Selection_MergeBranch,	"Merge change from the branch into the current project version" },
		{ "Selection_Branch",		&Commander::Selection_Branch,		&Commander::can_Selection_Branch,		"Create project branch starting from this version" },
		{ "Selection_VersionExport",&Commander::Selection_VersionExport,&Commander::can_Selection_VersionExport,"Export old version of file(s)" },
		{ "Selection_Archive",		&Commander::Selection_Archive,		&Commander::can_Selection_Archive,	    "Archive all versions before selection" },
		{ "Selection_UnArchive",	&Commander::Selection_UnArchive,	&Commander::can_Selection_UnArchive,	"Open archive" },
		{ "Selection_Add2FileFilter",&Commander::Selection_Add2FileFilter,	&Commander::can_Selection_Add2FileFilter,"Don't show files with the selected extension(s)" },
		{ "Selection_RemoveFromFileFilter",	&Commander::Selection_RemoveFromFileFilter,	&Commander::can_Selection_RemoveFromFileFilter,	"Remove the selected file extension(s) from the file filter" },
		{ "Selection_HistoryFilterAdd", &Commander::Selection_HistoryFilterAdd,	&Commander::can_Selection_HistoryFilterAdd,	"Filter history by selected files"			},
		{ "Selection_HistoryFilterRemove", &Commander::Selection_HistoryFilterRemove,	&Commander::can_Selection_HistoryFilterRemove,	"Clear history filter"	},
		{ "Selection_SaveFileVersion",	&Commander::Selection_SaveFileVersion,	&Commander::can_Selection_SaveFileVersion, "Save version of file(s)"		},
		{ "Selection_RestoreFileVersion",	&Commander::Selection_RestoreFileVersion,	&Commander::can_Selection_RestoreFileVersion, "Restores version of file(s)"		},
		{ "Selection_MergeFileVersion",	&Commander::Selection_MergeFileVersion,	&Commander::can_Selection_MergeFileVersion, "Merges version of file(s) into the current project version" },
		{ "Selection_AutoMergeFileVersion",	&Commander::Selection_AutoMergeFileVersion,	&Commander::can_Selection_AutoMergeFileVersion, "Automatically merges version of file(s) into the current project version" },
		{ "Selection_CopyList",		&Commander::Selection_CopyList,		&Commander::can_Selection_CopyList,	"Copy Select List To Clipboard"	},
		{ "Selection_Edit",			&Commander::Selection_Edit,			&Commander::can_Selection_Edit,		"Edit File" },
		{ "Selection_Next",			&Commander::Selection_Next,			&Commander::can_Selection_Next,		"Next Page" },
		{ "Selection_Previous",		&Commander::Selection_Previous,		&Commander::can_Selection_Previous,	"Previous Page" },
		{ "Selection_Home",			&Commander::Selection_Home,			&Commander::can_Selection_Home,		"Browse Home" },
		{ "Selection_Reload",		&Commander::Selection_Reload,		&Commander::can_Selection_Reload,	"Reload Page" },
		{ "Selection_CopyScriptComment", &Commander::Selection_CopyScriptComment, &Commander::can_Selection_CopyScriptComment,	"Copy Script Comment To Clipboard" },

		{ "Tools_Differ",			&Commander::Tools_Differ,			0,		"Configure Differ Tool" },
		{ "Tools_Merger",			&Commander::Tools_Merger,			0,		"Configure Merger Tool" },
		{ "Tools_Editor",			&Commander::Tools_Editor,			0,		"Configure Editor Tool" },
		{ "Tools_CreateBackup",		&Commander::Tools_CreateBackup,		&Commander::can_Tools_CreateBackup,	"Backup all projects" },
		{ "Tools_RestoreFromBackup",&Commander::Tools_RestoreFromBackup,&Commander::can_Tools_RestoreFromBackup,	"Restore all projects from backup" },
		{ "Tools_MoveToMachine",	&Commander::Tools_MoveToMachine,	0,	    "Move Code Co-op to a new machine" },

		{ "Help_Contents",			&Commander::Help_Contents,			0,		"Display Help" },
		{ "Help_Index",				&Commander::Help_Index,				0,		"Display Help Index" },
		{ "Help_Tutorial",			&Commander::Help_Tutorial,			0,		"Step-by-step tutorial" },
		{ "Help_Support",			&Commander::Help_Support,			0,		"On-line support" },
		{ "Help_SaveDiagnostics",	&Commander::Help_SaveDiagnostics,	0,		"Save diagnostic information to file" },
		{ "Help_RestoreOriginal",	&Commander::Help_RestoreOriginal,   &Commander::can_Help_RestoreOriginal, "Uninstall the current temporary version and restore the original version" },
		{ "Help_BeginnerMode",		&Commander::Help_BeginnerMode,		&Commander::can_Help_BeginnerMode,	"Display helpful hints for the inexperienced user" },

		// Commands triggered by control notification handlers
		{ "DoNewFolder",			&Commander::DoNewFolder,			0,									"" },
		{ "DoRenameFile",			&Commander::DoRenameFile,			0,									"" },
		{ "DoDrag",					&Commander::DoDrag,					&Commander::can_Drag,				"" },
		{ "DoDrop",					&Commander::DoDrop,					0,									"" },
		{ "SetCurrentFolder",		&Commander::SetCurrentFolder,		0,									"" },
		{ "DoNewFile",				&Commander::DoNewFile,				0,									"" },
		{ "DoDeleteFile",			&Commander::DoDeleteFile,			0,									"" },
		{ "Project_SelectMergeTarget",	&Commander::Project_SelectMergeTarget,	0,							"" },
		{ "Project_SetMergeType",	&Commander::Project_SetMergeType,	0,									"" },
		{ "Selection_OpenHistoryDiff",	&Commander::Selection_OpenHistoryDiff,	0,							"" },

		// Commands not associated with any menu item, but used by SccDll or others
		{ "GoBrowse",					&Commander::GoBrowse,					0, "" },
		{ "OnBrowse",					&Commander::OnBrowse,					0, "" },
		{ "Navigate",					&Commander::Navigate,					0, "" },
		{ "OpenFile",					&Commander::OpenFile,					0, "" },
		{ "CreateRange",				&Commander::CreateRange,				0, "" },
		{ "Selection_OpenCheckInDiff",	&Commander::Selection_OpenCheckInDiff,	0, "" },
		{ "RefreshMailbox",				&Commander::RefreshMailbox,				0, "" },
		{ "RestoreVersion",				&Commander::RestoreVersion,				0, "" },
		{ "Maintenance",				&Commander::Maintenance,				0, "" },
		{ "ReCreateFile",				&Commander::ReCreateFile,				0, "" },
		{ "MergeAttributes",			&Commander::MergeAttributes,			0, "" },
#if !defined (NDEBUG)
		{ "RestoreOneVersion",			&Commander::RestoreOneVersion,			0, "" },
#endif

		{ 0, 0, 0 }
	};
}
