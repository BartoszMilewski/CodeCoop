#if !defined (CMDVECTOR_H)
#define CMDVECTOR_H
//----------------------------
// (c) Reliable Software, 2004
//----------------------------
#include <Ctrl/Command.h>

class Commander;

namespace Cmd
{
	extern Cmd::Item<Commander> const Table [];
}

typedef Cmd::VectorExec<Commander> CmdVector;

#endif
