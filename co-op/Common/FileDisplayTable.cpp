//------------------------------------
//  (c) Reliable Software, 2004 - 2006
//------------------------------------

#include "precompiled.h"
#include "FileDisplayTable.h"

std::string FileDisplayTable::GetStateName (ChangeState state)
{
	switch (state)
	{
	case FileDisplayTable::Unchanged:
		return "Unchanged";
		break;
	case FileDisplayTable::Changed:
		return "Edited";
		break;
	case FileDisplayTable::Renamed:
		return "Renamed";
		break;
	case FileDisplayTable::Moved:
		return "Moved";
		break;
	case FileDisplayTable::Deleted:
		return "Deleted";
		break;
	case FileDisplayTable::Created:
		return "New";
		break;
	case FileDisplayTable::NotPresent:
		return "Not present";
		break;
	case FileDisplayTable::Unrecoverable:
		return "Unrecoverable";
		break;
	default:
		return "Unknown";
		break;
	}
}

