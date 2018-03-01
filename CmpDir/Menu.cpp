//-----------------------------------------
// (c) Reliable Software 1997, 98, 99, 2000
//-----------------------------------------

#include "precompiled.h"
#include "Menu.h"
#include <Ctrl/Menu.h>

namespace Menu
{
	//
	// Menu Popup Items
	//

	const Item programItems [] =
	{
		{CMD,		"&Exit",			"Program_Exit", 0},
		{END,		0,					0, 0}
	};

	const Item viewItems [] =
	{
		{CMD,		"&Old Tree",		"View_Old", 0},
		{CMD,		"&New Tree",		"View_New", 0},
		{CMD,		"&Diff",			"View_Diff", 0},
		{END,		0,					0, 0}
	};

	const Item directoryItems [] =
	{
		{CMD,		"&Dir Up\tBksp",	"Directory_Up", 0},
		{CMD,		"&Refresh\tF5",		"Directory_Refresh", 0},
		{END,		0,					0, 0}
	};

	const Item selectionItems [] =
	{
		{CMD,		"&View \tEnter",	"Selection_View", 0},
		{END,		0,					0, 0}
	};


	//
	// Menu Bar Items
	//

	const Item barItems [] =
	{
		{POP,   "&Program",		"Program",	programItems},
		{POP,	"&View",		"View",		viewItems},
		{POP,   "&Directory",	"Directory",directoryItems},
		{POP,   "&Selection",	"Selection",selectionItems},
		{END,	0,				0,			0}
	};
};
