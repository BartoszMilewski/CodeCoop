//----------------------------------
// (c) Reliable Software 1998 - 2008
// ---------------------------------

#include "precompiled.h"
#include "MenuTable.h"

namespace Menu
{
	// Drop-down menus
	const Item programItems [] =
	{
	//   Type		DisplayName					CommandName
        {CMD,       "Co-op &Update",	        "Program_CoopUpdate", 0	    },
        {CMD,       "Co-op Update &Options...", "Program_CoopUpdateOptions", 0	},
		{SEPARATOR, "",							"", 0						    },
		{CMD,		"Distribution &License...",	"Program_DistributorLicense", 0 },
        {SEPARATOR, "",							"", 0						    },
        {CMD,       "Collaboration &Wizard...",	"Program_ConfigWizard", 0	},
        {CMD,       "&Collaboration Settings...","Program_Collaboration", 0	},
		{CMD,       "&E-mail Options...",		"Program_EmailOptions", 0	},
		{CMD,       "&Diagnostics...",			"Program_Diagnostics", 0	},
        {CMD,       "&About...",				"Program_About", 0			},
        {SEPARATOR, "",							"", 0						},
        {CMD,		"E&xit",					"Program_Exit", 0			},
		{END,		0,							0, 0						}
	};

	const Item viewItems [] =
	{
		{CMD,		"Go &Up",				"View_GoUp", 0					},
		{SEPARATOR,	0,						0, 0							},
		{CMD,		"&Quarantine",			"View_Quarantine", 0			},
		{CMD,		"&Alert Log",			"View_AlertLog", 0				},
		{CMD,		"&Public Inbox",		"View_PublicInbox", 0			},
		{CMD,		"M&embers",				"View_Members", 0				},
		{CMD,		"Remote &Hubs",			"View_RemoteHubs", 0			},
		{END,		0,						0, 0}
	};

	const Item allItems [] =
    {
		{CMD,       "&Release From Quarantine",	"All_ReleaseFromQuarantine", 0 },
        {CMD,       "Pull Scripts from &Hub",	"All_PullFromHub", 0		},
        {CMD,       "&Dispatch Now",			"All_DispatchNow", 0		},
        {SEPARATOR, "",							"", 0						},
        {CMD,       "&Get Email",				"All_GetMail", 0			},
        {CMD,       "&Send Email",				"All_SendMail", 0			},
        {SEPARATOR, "",							"", 0						},
        {CMD,       "&Purge Addresses...",		"All_Purge", 0				},
		{SEPARATOR, "",							"", 0						},
		{CMD,       "&View Alert Log",		"All_ViewAlertLog", 0		},
		{CMD,       "&Clear Alert Log",		"All_ClearAlertLog", 0		},
        {END,		0,							0, 0						}
    };

	const Item selectionItems [] =
    {
		{CMD,       "&Release From Quarantine",	"Selection_ReleaseFromQuarantine", 0		},
        {CMD,       "D&etails...",				"Selection_Details", 0		},
        {SEPARATOR, "",							"", 0						},
        {CMD,		"De&lete",					"Selection_Delete", 0		},
        {END,		0,							0, 0						}
    };

    const Item helpItems [] =
	{
		{CMD,       "&Contents\tF1",			"Help_Contents", 0			},
		{END,		0,							0, 0						}
	};

	// Top-level Menu Bar

	const Item barItems [] =
	{
	//   Type	DisplayName	  Name		   Drop-down menu
		{POP,   "&Program",	  "Program",   programItems},
        {POP,   "&View",       "View",     viewItems   },
        {POP,   "&All",       "All",       allItems },
        {POP,   "&Selection", "Selection", selectionItems },
		{POP,   "&Help",	  "Help",	   helpItems},	
		{END,   0,			  0,           0} // Sentinel
	};

	// Taskbar Icon Context Menu

	const Item contextItems [] =
    {
        {CMD,       "Collaboration &Wizard...",	"Program_ConfigWizard", 0	  },
        {CMD,       "&Collaboration Settings...","Program_Collaboration", 0 },
		{CMD,       "&E-mail Options...",		"Program_EmailOptions", 0	},
        {SEPARATOR, "",							"", 0                    },
        {CMD,       "&Get Email",				"All_GetMail", 0         },
        {CMD,       "&Send Email",				"All_SendMail", 0        },
        {CMD,       "Pull Scripts from &Hub",	"All_PullFromHub", 0	  },
        {CMD,       "Dispatch &Now",			"All_DispatchNow", 0	  },
        {SEPARATOR, "",							"", 0                    },
        {CMD,       "Co-op &Update",	        "Program_CoopUpdate", 0  },
        {CMD,       "Co-op Update &Options...", "Program_CoopUpdateOptions", 0},
		{SEPARATOR, "",							"", 0                    },
		{CMD,		"&Distribution License...",	"Program_DistributorLicense", 0},
        {SEPARATOR, "",							"", 0                    },
        {CMD,       "&View Diagnostics",        "Window_Show", 0         },
        {CMD,       "&Run Diagnostics...",		"Program_Diagnostics", 0 },
        {SEPARATOR, "",							"", 0                    },
        {CMD,       "E&xit",					"Program_Exit", 0        },
        {END,       0,							0, 0                     }
    };
};
