//----------------------------------
// (c) Reliable Software 1997 - 2006
//----------------------------------

#include "precompiled.h"
#include "ButtonTable.h"

namespace Tool
{
	// In the order in which they appear in the bitmap
	enum ButtonID
	{
		btnDelete,
		btnRefresh,
		btnCut,
		btnPaste,
		btnCopy,
		btnNextChange,
		btnPrevChange,
		btnOpen,
		btnSave,
		btnCheckOut,
		btnUncheckOut,
		btnWrap,
		btnUndo,
		btnRedo,
		btnInfo,
		btnNew
	};

	Tool::Item Buttons [] =
	{
		{btnOpen,		"File_Open",			"Open File"							},
		{btnNew,		"File_New",				"Create New File (Ctrl+N)"			},
		{btnSave,		"File_Save",			"Save File (Ctrl+S)"				},
		{btnRefresh,	"File_Refresh",			"Refresh File (Ctrl+R)"				},
		{btnWrap,		"Edit_ToggleLineBreaking","Line Wrap"						},
		{btnCheckOut,	"File_CheckOut",		"Check-Out File (Ctrl+O)"			},
		{btnUncheckOut,	"File_UncheckOut",		"Uncheck-Out File (Ctrl+U)"			},
		{btnUndo,		"Edit_Undo",			"Undo (Ctrl + Z)"					},
		{btnRedo,		"Edit_Redo",			"Redo (Ctrl + Y)"					},
		{btnCut,		"Edit_Cut",				"Cut (Ctrl+X)"						},
		{btnCopy,		"Edit_Copy",			"Copy (Ctrl+C)"						},
		{btnPaste,      "Edit_Paste",			"Paste (Ctrl+V)"					},
		{btnDelete,     "Edit_Delete",			"Delete (Del)"						},
		{btnNextChange,	"Search_NextChange",	"Goto Next Change (F4)"				},
		{btnPrevChange,	"Search_PrevChange",	"Goto Previous Change (Shift+F4)"	},
		{Item::idEnd,	0,						0									},
	};


	// Different button layouts

	const int LayoutDifferButtons [] =
	{ btnOpen,	btnNew,	btnSave, btnRefresh, btnWrap, Item::idSeparator,
	  btnCheckOut, btnUncheckOut, Item::idSeparator,
	  btnUndo, btnRedo, btnCut, btnCopy, btnPaste, btnDelete, Item::idSeparator,
	  btnNextChange, btnPrevChange, Item::idEnd };
	const int LayoutNoButons [] =
	{ Item::idEnd};

	// Table of button layouts indexed by Tool::BarLayout enumeration

	int const * LayoutTable [] =
	{
		LayoutDifferButtons,
		0
	};

	// Template - list of all possible tool bands: 
	// {tool band id, button layout, extra space (in pixels) }
	Tool::BandItem Bands [] =
	{
		{ DifferBandId,			LayoutDifferButtons,	0	},
		{ FileDetailsBandId,	LayoutNoButons,			400	},
		{ InvalidBandId,		LayoutNoButons,			0	}
	};

	// Different tool band layouts

	const unsigned int DifferBands [] = { DifferBandId, FileDetailsBandId, InvalidBandId };

	// Table of tool band layouts indexed by Tool::BarLayout enumeration

	unsigned int const * BandLayoutTable [] =
	{
		DifferBands,
		0
	};
};
