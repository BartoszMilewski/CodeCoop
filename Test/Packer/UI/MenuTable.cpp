//-----------------------------------------
// (c) Reliable Software 2001
//-----------------------------------------

#include "precompiled.h"
#include "MenuTable.h"

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item testItems [] =
	{
		{CMD,		"&Run Test ...",	    "Test_Run", 0},		// Test
		{SEPARATOR,	0,						0, 0},					// -------
		{CMD,		"E&xit",				"Test_Exit", 0},		// Exit
		{END,		0,						0, 0}
	};

	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "Test",			"Test",		testItems},			//Test
		{END,   0,				0,			0}
	};
};
