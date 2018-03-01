//
// (c) Reliable Software 1999 -- 2002
//
//////////////////////////////////////////////////////

#include <WinLibBase.h>
#include "Atom.h"

Win::GlobalAtom::String::String (unsigned atom)
{
	char nameBuf [256];
	memset (nameBuf, 0, sizeof (nameBuf));
	int nameLen = ::GlobalGetAtomName (atom, nameBuf, sizeof (nameBuf));
	if (nameLen != 0)
	{
		assign (nameBuf);
	}
}
