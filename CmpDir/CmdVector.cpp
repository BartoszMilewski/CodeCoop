//------------------------------------
//  (c) Reliable Software, 2002
//------------------------------------
#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

namespace Cmd
{
	// Differ Commands

	Cmd::Item<Commander> const Table [] =
	{
																											
		{ "Program_Exit",	&Commander::Program_Exit,	&Commander::can_Program_Exit,   "Exit program" },
		{ "View_Old",	&Commander::View_Old,	&Commander::can_View_Old,   "View Old Tree" },
		{ "View_New",	&Commander::View_New,	&Commander::can_View_New,   "View New Tree" },
		{ "View_Diff",	&Commander::View_Diff,	&Commander::can_View_Diff,   "View Differences" },
		{ "Directory_Up",	&Commander::Directory_Up,	&Commander::can_Directory_Up,   "Go up one level" },
		{ "Directory_Refresh",	&Commander::Directory_Refresh,	0,   "Refresh View" },
		{ "Selection_View",	&Commander::Selection_View,	&Commander::can_Selection_View,	"View selected file/folder" },
		{ 0, 0, 0 }
	};
}