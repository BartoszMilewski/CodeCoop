// ----------------------------------
// (c) Reliable Software, 2001 - 2006
// ----------------------------------
#include "precompiled.h"
#include "ToolTable.h"

namespace Tool
{
	// In the order in which they appear in the bitmap
	enum ButtonID
	{
		btnUpArrow,
		btnDetails,
		btnDelete,
		btnReleaseAll,
		btnReleaseSelected,
		btnViewAlertLog,
		btnClearAlertLog
	};

	Item Buttons [] =
	{
		{btnReleaseSelected,  "Selection_ReleaseFromQuarantine","Release Selected Script(s) From Quarantine"	},
			{Item::idSeparator, 0, 0 },
		{btnReleaseAll,  "All_ReleaseFromQuarantine",	"Release All Scripts From Quarantine"	},
			{Item::idSeparator, 0, 0 },
		{btnViewAlertLog,  "All_ViewAlertLog",	"View Alert Log"	},
			{Item::idSeparator, 0, 0 },
		{btnClearAlertLog,  "All_ClearAlertLog",	"Clear Alert Log"	},
			{Item::idSeparator, 0, 0 },
		{btnDetails,    "Selection_Details",	"Display selection details"	},
			{Item::idSeparator, 0, 0 },
		{btnDelete,     "Selection_Delete",		"Delete selected items"		},
			{Item::idSeparator, 0, 0 },
		{btnUpArrow,    "View_GoUp",			"Go up"			},
			{Item::idEnd, 0, 0 }
	};

	const int QuarantineLayout  [] = { btnReleaseAll, btnReleaseSelected, Item::idSeparator, btnDelete, Item::idEnd };
	const int AlertLogLayout    [] = { btnViewAlertLog, Item::idSeparator, btnClearAlertLog, Item::idEnd };
	const int PublicInboxLayout [] = { btnDetails, btnDelete, Item::idEnd };
	const int MemberLayout		[] = { btnDetails, btnDelete, Item::idEnd };
	const int RemoteHubLayout   [] = { btnDetails, btnDelete, Item::idEnd };
	const int ProjectMemberLayout [] = { btnDetails, btnDelete, btnUpArrow, Item::idEnd };

	int const * LayoutTable [] = 
	{
		QuarantineLayout,
		AlertLogLayout,
		PublicInboxLayout,
		MemberLayout,
		RemoteHubLayout,
		ProjectMemberLayout,
		0
	};
};
