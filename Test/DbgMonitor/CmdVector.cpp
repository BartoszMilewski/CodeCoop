//----------------------------------
// (c) Reliable Software 2003 - 2006
//----------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

// Debug Monitor Commands

namespace Cmd
{
	const Cmd::Item<Commander> Table [] =
	{
		{ "Monitor_Save",	&Commander::Monitor_Save,	&Commander::can_Monitor_Save,	"Save debug output to file"},
		{ "Monitor_ClearAll",&Commander::Monitor_ClearAll,&Commander::can_Monitor_ClearAll,	"Clear all debug output"},
		{ "Monitor_Exit",	&Commander::Monitor_Exit,	&Commander::can_Monitor_Exit,	"Exit program"},
		{ 0, 0, 0 }
	};
}
