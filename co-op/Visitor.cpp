//----------------------------------
// (c) Reliable Software 1997 - 2007
//----------------------------------

#include "precompiled.h"
#include "Visitor.h"
#include "DataBase.h"
#include "PathFind.h"
#include "SynchArea.h"
#include "OutputSink.h"

#include <Ctrl/ProgressMeter.h>
#include <Ex/WinEx.h>
#include <File/File.h>


void OriginalVerifier::Visit (GlobalId gid)
{
    FileData const * fileData = _dataBase.GetFileDataByGid (gid);
    Assert (fileData != 0);
    Assert (!fileData->GetType ().IsFolder ());
    if (!fileData->GetState ().IsPresentIn (Area::Original))
    {
        char const * fullPath = _pathFinder.GetFullPath (gid, Area::Original);
		File::DeleteNoEx (fullPath);
    }
}

void OriginalBackupVerifier::Visit (GlobalId gid)
{
    char const * fullPath = _pathFinder.GetFullPath (gid, Area::OriginalBackup);
	File::DeleteNoEx (fullPath);
}

void SynchVerifier::Visit (GlobalId gid)
{
    FileData const * fileData = _dataBase.FindByGid (gid);
    bool deleteFile = false;
    if (fileData == 0)
    {
        deleteFile = true;
    }
    else
    {
        Assert (!fileData->GetType ().IsFolder ());
        if (!_synchArea.IsPresent (gid))
        {
            deleteFile = true;
        }
    }
    if (deleteFile)
    {
        char const * fullPath = _pathFinder.GetFullPath (gid, Area::Synch);
		File::DeleteNoEx (fullPath);
    }
}

void ReferenceVerifier::Visit (GlobalId gid)
{
    FileData const * fileData = _dataBase.FindByGid (gid);
    bool deleteFile = false;
    if (fileData == 0)
    {
        deleteFile = true;
    }
    else
    {
        Assert (!fileData->GetType ().IsFolder ());
        if (!_synchArea.IsPresent (gid))
        {
            deleteFile = true;
        }
    }
    if (deleteFile)
    {
        char const * fullPath = _pathFinder.GetFullPath (gid, Area::Reference);
		File::DeleteNoEx (fullPath);
    }
}

void StagingVerifier::Visit (GlobalId gid)
{
	FileData const * fileData = _dataBase.FindByGid (gid);
	if (fileData == 0) // happens when aborted staging of newly added file
		return;

	if (fileData->GetType ().IsFolder ())
		throw Win::Exception ("Internal error: Corrupted database -- folder present in the staging area",
							  _pathFinder.GetFullPath (fileData->GetUniqueName ()));

	char const * stagingPath = _pathFinder.GetFullPath (gid, Area::Staging);
	if (_redo)
	{
		char const * projectPath = _pathFinder.GetFullPath (fileData->GetUniqueName ());

		try
		{
			CheckSum checkSum;
			if (fileData->GetState ().IsCheckedIn ())
			{
				checkSum = CheckSum (stagingPath);
			}
			else
			{
				Assert (fileData->GetState ().IsRelevantIn (Area::Original));
				checkSum = fileData->GetCheckSum ();
			}

			if (checkSum == fileData->GetCheckSum ())
			{
				// Change attributes before copying
				File::MakeReadWrite (stagingPath);
				File::Touch (stagingPath);
				if (fileData->GetState ().IsCheckedIn ())
					File::MakeReadOnly (stagingPath);

				File::Copy (stagingPath, projectPath);
			}
			else
			{
				std::string msg ("Internal error: File in the staging area has wrong checksum\n");
				msg += projectPath;
				msg += "\nPlease run Repair from the Project menu";
				TheOutput.Display (msg.c_str (), Out::Error);
#if !defined NDEBUG
				return; // don't delete when debugging
#endif
			}
		}
		catch (Win::Exception  ex)
		{
			TheOutput.Display (ex);
			Win::ClearError ();
			throw Win::ExitException ("The program will exit now.\n"
									  "Correct the problem and restart the program to fully recover from this error.\n"
									  "Make sure the path is accessible.",
									  projectPath);
		}
		catch (...)
		{
			Win::ClearError ();
			throw Win::ExitException ("Intermittent File Operation Problem\n\nProgram will exit now.\n"
									  "Correct the problem and restart the program to fully recover from this error.",
									  projectPath);
		}
	}
    File::DeleteNoEx (stagingPath);
}

bool ProjectVerifier::Visit (UniqueName const & uname, GlobalId & folderId)
{
	_meter.StepIt ();
    bool recurse = false;
	Assert (uname.IsValid ());
    FileData const * fileData = _dataBase.FindProjectFileByName (uname);
    if (fileData != 0)
    {
        if (fileData->GetType ().IsFolder ())
        {
			// Visit only folders belonging to the project
            folderId = fileData->GetGlobalId ();
            recurse = true;
        }
        else
        {
            FileState state = fileData->GetState ();
            GlobalId gid = fileData->GetGlobalId ();
            if (state.IsPresentIn (Area::Original))
            {
                char const * fullPath = _pathFinder.GetFullPath (gid, Area::Original);
                if (!File::Exists (fullPath))
                {
					std::string  info ("Corrupted database; Missing file original!\n'");
					info += fullPath;
					info += '\'';
					TheOutput.Display (info.c_str (), Out::Error);
                }
            }
            if (state.IsPresentIn (Area::Synch))
            {
                char const * fullPath = _pathFinder.GetFullPath (gid, Area::Synch);
                if (!File::Exists (fullPath))
                {
					std::string info ("Corrupted database; Missing sync copy of file!\n'");
					info += fullPath;
					info += '\''; 
					TheOutput.Display (info.c_str (), Out::Error);
                }
            }
            if (state.IsPresentIn (Area::PreSynch))
            {
                char const * fullPath = _pathFinder.GetFullPath (gid, Area::PreSynch);
                if (!File::Exists (fullPath))
                {
					std::string info ("Corrupted database; Missing pre-sync copy of file!\n'");
					info += fullPath;
					info += '\''; 
					TheOutput.Display (info.c_str (), Out::Error);
                }
            }
            if (state.IsPresentIn (Area::Reference))
            {
                char const * fullPath = _pathFinder.GetFullPath (gid, Area::Reference);
                if (!File::Exists (fullPath))
                {
					std::string info ("Corrupted database; Missing reference copy of file!\n'");
					info += fullPath;
					info += '\''; 
					TheOutput.Display (info.c_str (), Out::Error);
                }
            }
        }
    }
    return recurse;
}

void PreSynchVerifier::Visit (GlobalId gid)
{
    FileData const * fileData = _dataBase.GetFileDataByGid (gid);
    Assert (!fileData->GetType ().IsFolder ());
    if (!fileData->GetState ().IsRelevantIn (Area::Synch))
    {
        char const * fullPath = _pathFinder.GetFullPath (gid, Area::PreSynch);
		File::DeleteNoEx (fullPath);
    }
}

void TemporaryVerifier::Visit (GlobalId gid)
{
    char const * fullPath = _pathFinder.GetFullPath (gid, Area::Temporary);
	File::DeleteNoEx (fullPath);
}

void CompareVerifier::Visit (GlobalId gid)
{
    char const * fullPath = _pathFinder.GetFullPath (gid, Area::Compare);
	File::DeleteNoEx (fullPath);
}
