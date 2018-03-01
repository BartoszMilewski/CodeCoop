#if !defined CMDVECTOR
#define CMDVECTOR

//----------------------------------------
// (c) Reliable Software 2007-09
//----------------------------------------

#include <Ctrl/Command.h>

class Commander;

namespace Cmd
{
	extern Cmd::Item<Commander> const Table [];
}

typedef Cmd::VectorExec<Commander> CmdVector;

#endif
