//
// (c) Reliable Software 1997
//
#include "precompiled.h"
#include "SynchTrans.h"
#include "DataBase.h"
#include "PathFind.h"
#include <File/Dir.h>
#include <Ex/WinEx.h>

SynchTransaction::SynchTransaction (Transactable& xable, 
									PathFinder& pathFinder, 
									DataBase& dataBase,
									TransactionFileList & fileList)
    : FileTransaction (xable, pathFinder, fileList)
{
	std::string allPrevFiles (pathFinder.XGetAllFilesPath (Area::Original));
    dataBase.XSwitchOriginalAreaId ();
    std::string allCurFiles (pathFinder.XGetAllFilesPath (Area::Original));

    // Delete files thay may remain from the previous original area switch
    for (FileSeq cleanupSeq (allCurFiles.c_str ()); !cleanupSeq.AtEnd (); cleanupSeq.Advance ())
    {
        char const * cleanupPath = pathFinder.GetSysFilePath (cleanupSeq.GetName ());
		File::DeleteNoEx (cleanupPath);
    }

    // Copy files to the current original area
    for (FileSeq seq (allPrevFiles.c_str ()); !seq.AtEnd (); seq.Advance ())
    {
        GlobalIdPack gid (seq.GetName ());
        std::string oldPath (pathFinder.GetSysFilePath (seq.GetName ()));
        std::string newPath (pathFinder.XGetFullPath (gid, Area::Original));
        File::Copy (oldPath.c_str (),  newPath.c_str ());
    }
}

