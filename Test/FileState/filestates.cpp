// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------
#include "precompiled.h"
#include "FileStates.h"

// implementation

/*
	always be in synch with:
	- state definitions in Common\FileState.cpp
	- merge bit table   in co-op\FileTransform.cpp

	only 13 (out of 15) bits says about persistent file state
	bits coDiff (12) and soDiff (13) are volatile

	   P PS pso O  co R  ro S  so coDel soDel coDiff soDiff M  RST(not used in 4.5)
    { {0, 0, 0,	0, 0, 0, 0, 0, 0, 0,    0,					0	0	},	"None"           },
    { {1, 0, 0,	0, 0, 0, 0, 0, 0, 0,    0,					0	0	},	"In"             },
    { {1, 0, 0,	1, 1, 0, 0, 0, 0, 0,    0,					0	0	},	"Out"            },
    { {1, 0, 0,	0, 0, 0, 0, 0, 0, 0,    0,					0	0	},	"New"            },
    { {0, 0, 0,	1, 1, 0, 0, 0, 0, 1,    0,					0	0	},	"Delete"         },
    { {0, 0, 0,	1, 1, 0, 0, 0, 0, 0,    0,					0	0	},	"Remove"         },
    { {1, 1, 1,	0, 0, 0, 0, 1, 1, 0,    0,					0	0	},	"Sync"           },
    { {0, 1, 1,	0, 0, 1, 0, 0, 1, 0,    1,					0	0	},	"SyncDelete"     }, // File
	{ {0, 1, 1,	0, 0, 0, 0, 0, 1, 0,    1,					0	0	},	"SyncDelete"	 }, // Folder
    { {0, 1, 1,	0, 0, 1, 0, 0, 1, 0,    0,					0	0	},	"SyncRemove"     }, // File
	{ {0, 1, 1,	0, 0, 0, 0, 0, 1, 0,    0,					0	0	},	"SyncRemove"     }, // Folder
    { {1, 1, 1,	1, 1, 1, 1, 1, 1, 0,    0,					1	0	},	"Merge"          },
    { {0, 0, 1,	1, 1, 1, 1, 1, 1, 1,    0,					1	0	},	"MergeLocalDel  "},
    { {0, 0, 1,	1, 1, 1, 1, 1, 1, 0,    0,					1	0	},	"MergeLocalRemove"}
    { {1, 1, 1,	0, 1, 1, 1, 0, 1, 0,    1,					1	0	},	"MergeSyncDel"   },
    { {1, 1, 1,	0, 1, 1, 1, 0, 1, 0,    0,					1	0	},	"MergeSyncRemove"},
    
	{ {1, 1, 1,	1, 1, 1, 1, 1, 1, 1,    1,					1	1	},	0				 }
*/

const FileStateMap::Name2State FileStateMap::_stateTable [] =
{
	// name,				  value		mask
	{ "None",				{ 0,		0	}	},
	{ "New",				{ 0x11,     0x146}	},
	{ "In",					{ 0x1,      0	}	},
	{ "Out",				{ 0x19,     0	}	},
	{ "Remove",				{ 0x18,     0	}	},
	{ "Del",				{ 0x218,    0	}	},
	{ "Sync",				{ 0x187,    0x2	}	},
	{ "SyncRemove",			{ 0x106,    0x60}	},
	{ "SyncDel",			{ 0x506,    0x60}	},
	{ "Merge",				{ 0x1ff,    0x202a}	},
	{ "MergeLocalRemove",	{ 0x21fc,   0x2002}	},
	{ "MergeLocalDel",		{ 0x23fc,   0x2002}	},
	{ "MergeSyncRemove",	{ 0x2177,   0x2000}	},
	{ "MergeSyncDel",		{ 0x2577,   0x2000}	},
	{ 0,					{ 0,		0   }	}
};

FileStateMap::FileStateMap ()
{
	unsigned int stateTableSize = sizeof (_stateTable) / sizeof (FileStateMap::Name2State) - 1;
	for (unsigned int i = 0; i < stateTableSize; ++i)
	{
		_stateMap [_stateTable [i]._name] = _stateTable [i]._state;
	}
}

bool FileStateMap::GetState (
		std::string const & stateStr,
		unsigned long & state,
		unsigned long & unsignificantBitsMask)
{
	NocaseMap<State>::iterator result = _stateMap.find (stateStr);
	if (result == _stateMap.end ())
		return false;
	
	state = result->second._value;
	unsignificantBitsMask = result->second._mask;
	return true;
}
