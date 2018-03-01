//-----------------------------------------
// (c) Reliable Software 2001 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "CmdVector.h"
#include "Commander.h"

// Packer Test Commands

namespace Cmd
{
	const Cmd::Item<Commander> Table [] =
	{
		{ "Test_Run",	&Commander::Test_Run,	&Commander::can_Test_Run,	"Run packer test"},
		{ "Test_Exit",	&Commander::Test_Exit,	&Commander::can_Test_Exit,	"Exit program"},
		{ 0, 0, 0 }
	};
}
