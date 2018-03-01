//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "ButtonTable.h"

namespace Tool
{
	// In the order in which they appear in the bitmap
	enum ButtonID
	{
		btnNewFile,
		btnCheckOut,
		btnUncheckout,
		btnCheckIn,
		btnCheckInAll,
		btnExecuteScript,
		btnSynch,
		btnSynchAll,
		btnOpen,
		btnMultiMerge,
		btnCheckOutAll,
		btnAddToProject,
		btnFolderUp,
		btnDelete,
		btnRefresh,
		btnCut,
		btnPaste,
		btnResend,
		btnCopy,
		btnHist,
		btnShowScriptComment,
		btnHideFiles,
		btnProperties,
		btnFilterAdd,
		btnFilterRemove,
		btnSaveChange,
		btnCopyList,
		btnEdit,
		btnUp,
		btnLeft,
		btnRight,
		btnGo,
		btnMergeFile,
		btnUnused,
		btnClosePage,
		btnMergeScript,
		btnOpenDiffer,
		btnAutoMergeFile,
		btnReload
	};

	// Template--list of all possible tool bar buttons: 
	// {button id, command name, tool tip text}

	Tool::Item Buttons [] =
	{
		{btnNewFile,			"Folder_NewFile",					"Create new file"               },
		{btnCheckOut,			"Selection_CheckOut",				"Check out selected file(s)"	},
		{btnUncheckout,			"Selection_Uncheckout",				"Uncheckout selected files"     },
		{btnCheckIn,			"Selection_CheckIn",				"Check in (commit) selected file(s)" },
		{btnCheckInAll,			"All_CheckIn",						"Check in (commit) all files"   },
		{btnExecuteScript,		"Selection_Synch",					"Execute next script to synchronize project" },
		{btnSynch,				"Selection_AcceptSynch",			"Accept changes in selected file(s)" },
		{btnSynchAll,			"All_AcceptSynch",					"Accept changes in all files"   },
		{btnProperties,			"Selection_Properties",				"Properties"					},
		{btnOpen,				"Selection_Open",					"Open selected item"            },
		{btnCheckOutAll,		"All_CheckOut",						"Check out all files"           },
		{btnAddToProject,		"Selection_Add",					"Add selected file(s) to project" },
		{btnFolderUp,			"Folder_GoUp",						"Go to parent folder"           },
		{btnDelete,				"Selection_Delete",					"Delete selected file(s)"       },
		{btnRefresh,			"View_Refresh",						"Refresh current view"          },
		{btnCut,				"Selection_Cut",					"Cut selected file(s) for move" },
		{btnPaste,				"Selection_Paste",					"Paste (move) files"            },
		{btnResend,				"Selection_SendScript",				"Re-send selected script(s)"    },
		{btnCopy,				"Selection_Copy",					"Copy selected file(s) to clipboard"},
		{btnHist,				"Selection_ShowHistory",			"View history of selected item(s)"},
		{btnHideFiles,			"View_ApplyFileFilter",				"Apply file filter"				},
		{btnFilterAdd,			"Selection_HistoryFilterAdd",		"Filter history by selected files"},
		{btnFilterRemove,		"Selection_HistoryFilterRemove",	"Turn off history filter"	},
		{btnSaveChange,			"Selection_SaveFileVersion",		"Save version of file(s)"		},
		{btnMergeFile,			"Selection_MergeFileVersion",		"Merge branch files(s)"			},
		{btnMergeScript,		"View_MergeProjects",				"Open branch merge view"		},
		{btnCopyList,			"Selection_CopyList",				"Copy selected list to clipboard"},
		{btnEdit,				"Selection_Edit",					"Edit file"						},
		{btnUp,					"Selection_Home",					"Home page"						},
		{btnReload,				"Selection_Reload",					"Reload page"					},
		{btnLeft,				"Selection_Previous",				"Page back"						},
		{btnRight,				"Selection_Next",					"Page forward"					},
		{btnGo,					"GoBrowse",							"Navigate to"					},
		{btnClosePage,			"View_ClosePage",					"Close this view"				},
		{btnOpenDiffer,			"Selection_OpenHistoryDiff",		"View source branch changes"	},
		{btnAutoMergeFile,		"Selection_AutoMergeFileVersion",	"Automatically merge branch files(s)"	},
		{Tool::Item::idEnd, 0, 0 }	  
	};

	// Different button layouts

	const int LayoutProjects [] =
		{ btnOpen,	btnRefresh,	Item::idEnd };
	const int LayoutBrowser [] =
		{ btnEdit, btnLeft, btnUp, btnRight, btnReload, Item::idEnd };
	const int LayoutFiles [] =
		{ btnCheckOut, btnCheckOutAll, btnUncheckout, btnNewFile, btnAddToProject, btnOpen, btnHist, Item::idSeparator, btnDelete, btnCut, btnCopy, btnPaste, Item::idSeparator, btnRefresh, btnHideFiles, Item::idEnd };
	const int LayoutMailbox [] = 
		{ btnExecuteScript, btnOpen, btnDelete, Item::idSeparator, btnRefresh, Item::idEnd };
	const int LayoutCheckIn [] =
		{ btnCheckInAll, btnCheckIn, btnUncheckout, btnOpen, btnHist, Item::idSeparator, btnRefresh, Item::idEnd};
	const int LayoutSynch [] =
		{ btnSynchAll, btnSynch, btnCheckOut, btnCheckOutAll, btnUncheckout, btnOpen, Item::idSeparator, btnRefresh, Item::idEnd};
	const int LayoutHistory [] =
		{ btnMergeScript, btnOpen, btnProperties, btnResend, Item::idEnd};
	const int LayoutCurrentFolder [] =
		{ btnFolderUp, Item::idEnd};
	const int LayoutHistoryScriptDetails [] =
		{ btnMergeFile, btnAutoMergeFile, btnOpen, btnSaveChange, btnFilterAdd, btnFilterRemove, btnCopyList, Item::idEnd };
	const int LayoutFileMergeDetails [] =
		{ btnMergeFile, btnAutoMergeFile, Item::idEnd };
	const int LayoutProjectMergeDetails [] =
		{ btnOpenDiffer, btnSaveChange, btnFilterAdd, btnFilterRemove, btnCopyList, Item::idSeparator, btnRefresh, Item::idEnd };
	const int LayoutUrl [] =
		{ btnGo, Item::idEnd};
	const int LayoutProjectMerge [] =
		{ btnClosePage, Item::idEnd};
	const int LayoutNoButtons [] =
		{ Item::idEnd};

	// Table of button layouts indexed by Tool::BarLayout enumeration

	int const * ButtonLayoutTable [] =
	{
		// Button layouts for bands appearing on the main instrument bar
		LayoutNoButtons,
		LayoutProjects,
		LayoutBrowser,
		LayoutFiles,
		LayoutMailbox,
		LayoutCheckIn,
		LayoutSynch,
		LayoutHistory,
		LayoutProjectMerge,

		// Button layouts for bands appearing on the auxiliary instrument bars
		LayoutHistoryScriptDetails,
		LayoutProjectMergeDetails,
		LayoutFileMergeDetails,
		0
	};

	// Template - list of all possible tool bands that can appear in the main instrument bar: 
	// {tool band id, button layout, extra space (in pixels) }
	Tool::BandItem Bands [] =
	{
		{ NotInProjectBandId,	LayoutNoButtons,		0	},
		{ BrowserBandId,		LayoutBrowser,			0	},
		{ FilesBandId,			LayoutFiles,			0	},
		{ CheckinBandId,		LayoutCheckIn,			0	},
		{ MailboxBandId,		LayoutMailbox,			0	},
		{ SyncBandId,			LayoutSynch,			0	},
		{ HistoryBandId,		LayoutHistory,			0	},
		{ ProjectBandId,		LayoutProjects,			0	},
		{ ScriptCommentBandId,	LayoutNoButtons,		300	},
		{ HistoryFilterBandId,	LayoutNoButtons,		300	},
		{ CurrentFolderBandId,	LayoutCurrentFolder,	250	},
		{ UrlBandId,			LayoutUrl,				250 },
		{ ProjectMergeBandId,	LayoutProjectMerge,		0   },
		{ CheckinStatusBandId,	LayoutNoButtons,		250	},
		{ InvalidBandId,		LayoutNoButtons,		0	}
	};

	// Template - list of all possible tool bands that can appear in the auxiliary
	// instrument bar in the mailbox and history page: 
	// {tool band id, button layout, extra space (in pixels) }
	Tool::BandItem ScriptBands [] =
	{
		{ ScriptDetailsBandId,	LayoutHistoryScriptDetails,	0	},
		{ ScriptCommentBandId,	LayoutNoButtons,			300	},
		{ InvalidBandId,		LayoutNoButtons,			0	}
	};

	// Template - list of all possible tool bands that can appear in the auxiliary
	// instrument bar in the project merge page: 
	// {tool band id, button layout, extra space (in pixels) }
	Tool::BandItem ProjectMergeBands [] =
	{
		{ TargetProjectBandId,	LayoutNoButtons,			150	},
		{ MergeTypeBandId,		LayoutNoButtons,			120	},
		{ MergeDetailsBandId,	LayoutFileMergeDetails,		0	},
		{ ScriptDetailsBandId,	LayoutProjectMergeDetails,	0	},
		{ MergedVersionBandId,	LayoutNoButtons,			200	},
		{ InvalidBandId,		LayoutNoButtons,			0	}
	};

	// Different tool band sets
	// For main instrument bar
	const unsigned int NotInProjectBands [] = { InvalidBandId };
	const unsigned int ProjectBands [] = { ProjectBandId, InvalidBandId };
	const unsigned int BrowserBands [] = { BrowserBandId, UrlBandId, InvalidBandId };
	const unsigned int BrowsingFilesBands [] = { FilesBandId, CurrentFolderBandId, InvalidBandId };
	const unsigned int MailboxBands [] = { MailboxBandId, ScriptCommentBandId, InvalidBandId }; 
	const unsigned int CheckInBands [] = { CheckinBandId, CheckinStatusBandId, InvalidBandId };
	const unsigned int SyncBands [] = { SyncBandId, ScriptCommentBandId, InvalidBandId };
	const unsigned int HistoryBands [] = { HistoryBandId, HistoryFilterBandId, InvalidBandId };
	const unsigned int ProjectMergeHistoryBands [] = { ProjectMergeBandId, HistoryFilterBandId, InvalidBandId };

	// For auxilary instrument bars
	const unsigned int HistoryScriptBands [] = { ScriptDetailsBandId, ScriptCommentBandId, InvalidBandId };
	const unsigned int ProjectMergeToolBands [] = { TargetProjectBandId, MergeTypeBandId, MergeDetailsBandId, ScriptDetailsBandId, MergedVersionBandId, InvalidBandId };

	// Table of tool band layouts indexed by Tool::BarLayout enumeration

	unsigned int const * BandLayoutTable [] =
	{
		// Main instrument bar band sets
		NotInProjectBands,
		ProjectBands,
		BrowserBands,
		BrowsingFilesBands,
		MailboxBands,
		CheckInBands,
		SyncBands,
		HistoryBands,
		ProjectMergeHistoryBands,

		// Auxilary instrument bar band sets
		HistoryScriptBands,
		ProjectMergeToolBands,
		0
	};
};
