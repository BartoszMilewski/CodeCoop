// ---------------------------
// (c) Reliable Software, 2001
// ---------------------------

#include "precompiled.h"
#include "AccelTable.h"
#include <Win/Keyboard.h>

namespace Accel
{
	const Accel::Item Keys [] =
	{
	//   Flags				Key				Command Name
		{Plain,				VKey::F1,		"Help_Contents"			}, // Help
		{WithCtrl,			VKey::Return,	"Selection_Details"		}, // Details
		{WithCtrl,			VKey::Tab,		"View_Next"				}, // Switch view tabs
		{WithCtrlAndShift,	VKey::Tab,		"View_Previous"			}, // Switch view tabs
		{ 0, 0, 0 } // Sentinel
	};
}
