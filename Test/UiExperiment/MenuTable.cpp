//----------------------------------
// (c) Reliable Software 2007-09
//----------------------------------

#include "precompiled.h"
#include "MenuTable.h"

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item programItems [] =
	{
		{CMD,		"E&xit",			"Browser_Exit", 0},		// Exit
		{END,		0,					0, 0}
	};

	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "Program",		"Program",	programItems},		// Program
		{END,   0,				0,			0}
	};
};
