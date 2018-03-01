//----------------------------------
// (c) Reliable Software 2003 - 2006
//----------------------------------

#include "precompiled.h"
#include "MenuTable.h"

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item monitorItems [] =
	{
		{CMD,		"&Save ...",	    "Monitor_Save", 0},		// Save
		{CMD,		"&Clear All",	    "Monitor_ClearAll", 0},	// Clear All
		{SEPARATOR,	0,					0, 0},					// -------
		{CMD,		"E&xit",			"Monitor_Exit", 0},		// Exit
		{END,		0,					0, 0}
	};

	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "Monitor",		"Monitor",	monitorItems},		// Monitor
		{END,   0,				0,			0}
	};
};
