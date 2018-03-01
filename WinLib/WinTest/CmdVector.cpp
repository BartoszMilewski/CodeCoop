//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

namespace Cmd
{
	Cmd::Item<Commander> const Table [] =
	{																									
		{ "Program_Exit",	&Commander::Program_Exit,	&Commander::can_Program_Exit,   "Exit program" },
		{ "Program_About",	&Commander::Program_About,	0,   "About" },
		{ "Dialogs_Modeless",	&Commander::Dialogs_Modeless,	0,   "Modeless Dialog" },
		{ "Dialogs_Listview",	&Commander::Dialogs_Listview,	0,   "Dialog with Listview Control" },
		{ 0, 0, 0 }
	};
}