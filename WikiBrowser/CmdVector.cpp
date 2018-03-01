//----------------------------------
// (c) Reliable Software 2007
//----------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

// Wiki Browser Commands

namespace Cmd
{
	const Cmd::Item<Commander> Table [] =
	{
		{ "Browser_Exit",	&Commander::Browser_Exit,	0,	"Exit program"},
		{ 0, 0, 0 }
	};
}
