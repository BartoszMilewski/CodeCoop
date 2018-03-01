//---------------------------
// (c) Reliable Software 1998
//---------------------------

#include <WinLibBase.h>
#include "Command.h"

using namespace Cmd;

// Returns -1 when command not found
int Vector::GetIndex (char const * cmdName) const
{
	int cmdIndex = -1;
	CmdMap::const_iterator iter = _cmdMap.find (cmdName);
	if (iter != _cmdMap.end ())
		cmdIndex = iter->second;
	return cmdIndex;
}

// Returns -1 when command not found
int Vector::Cmd2Id (char const * cmdName) const
{
	int idx = GetIndex (cmdName);
	if (idx != -1)
		return idx + cmdIDBase;
	else
		return -1;
}

int Vector::Id2Index (int id) const
{
    return id - cmdIDBase;
}

