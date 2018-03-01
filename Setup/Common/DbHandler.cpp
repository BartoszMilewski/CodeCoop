//-----------------------------------------
//  (c) Reliable Software, 2005
//-----------------------------------------

#include "precompiled.h"
#include "DbHandler.h"

#include <File/File.h>
#include <Com/Shell.h>

char const * CoopDbHandler::_folderNames [] = {
												"PublicInbox",
												"Inbox",
												"Database",
												"Updates",
												"Logs",
												"Wiki",
												""
											  };

bool CoopDbHandler::CreateDb ()
{
	for (unsigned int i = 0; *_folderNames [i] != '\0'; ++i)
	{
		if (!File::CreateFolder (_root.GetFilePath (_folderNames [i])))
			return false;
	}
	return true;
}

void CoopDbHandler::DeleteDb ()
{
	for (unsigned int i = 0; *_folderNames [i] != '\0'; ++i)
	{
		ShellMan::QuietDelete (0, _root.GetFilePath (_folderNames [i]));
	}
	File::DeleteNoEx (_root.GetFilePath ("ErrorLog.html"));
	File::DeleteNoEx (_root.GetFilePath ("BackupMarker.bin"));
	File::DeleteNoEx (_root.GetFilePath ("RepairList.txt"));
	if (File::IsTreeEmpty (_root.GetDir ()))
		::RemoveDirectory (_root.GetDir ());
}
