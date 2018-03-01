//-----------------------------------------
// (c) Reliable Software 1997, 98, 99, 2000
//-----------------------------------------

#include "precompiled.h"
#include "AccelTab.h"

#include <Win/Keyboard.h>

namespace Accel
{
	const Accel::Item Keys [] =
	{
		//File
		{ WithCtrl,	VKey::S,		"File_Save"},	
		{ WithCtrl,	VKey::R,		"File_Refresh"},
		{ WithCtrl,	VKey::N,		"File_New"},

		//Edit
		{ WithCtrl,	VKey::Z,		"Edit_Undo"},
		{ WithCtrl,	VKey::Y,		"Edit_Redo"},
		{ WithCtrl,	VKey::C,		"Edit_Copy"},
		{ WithCtrl,	VKey::Insert,   "Edit_Copy"},
		{ WithCtrl,	VKey::X,		"Edit_Cut"},
		{ WithShift,VKey::Delete,	"Edit_Cut"},
		{ WithCtrl,	VKey::V,		"Edit_Paste"},
		{ WithShift,VKey::Insert,	"Edit_Paste"},
		{ WithCtrl,	VKey::A,		"Edit_SelectAll"},	
		{ Plain,	VKey::Delete,	"Edit_Delete"},	

		//Search
		{ Plain,	VKey::F4,		"Search_NextChange"},
		{ WithShift,VKey::F4,		"Search_PrevChange"},
		{ WithCtrl,	VKey::F,		"Search_FindText"},
		{ WithCtrl,	VKey::H,		"Search_Replace"},
		{ Plain,	VKey::F3,		"Search_FindNext"},
		{ WithShift,VKey::F3,	    "Search_FindPrev"},
		{ WithCtrl,	VKey::G,		"Search_GoToLine"},

		//Help
		{ Plain,	VKey::F1,		"Help_Contents"},

		//Keyboard Navigation
		{ Plain,	VKey::F6,		"Nav_NextPanel"},
		{ WithShift,VKey::F6,		"Nav_NextPanel"},

		{ 0, 0, 0 }
	};

	const Accel::Item KeysDialogFind [] =
	{
		{ Plain,	 VKey::F3,	"Search_FindNext"},
		{ WithShift, VKey::F3,	"Search_FindPrev"},
		{ 0, 0, 0 }
	};
}
