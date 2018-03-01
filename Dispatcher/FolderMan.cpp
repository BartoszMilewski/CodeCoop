//-------------------------------------
// (c) Reliable Software 1998-2006
// ------------------------------------
#include "precompiled.h"
#include "FolderMan.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "AlertMan.h"
#include <File/File.h>
#include <File/Path.h>
#include <LightString.h> // class Msg
#include <Com/Shell.h>
#include <Win/Win.h>
#include <Ctrl/Output.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>

class XFolderList
{
public:
    XFolderList () : _commit (false) {}
	~XFolderList ();
    void Commit () { _commit = true; }
    void RememberCreatedDir (char const * fullPath);
private:
    std::vector<std::string> _created;
    bool _commit;
};

void DisplayError (char const * what, char const * why, char const * srcFilename, char const * destFolder)
{
	std::string msg = "Copy failed:\n";
	msg += "From: ";
	msg += srcFilename;
	msg += "\n";
	msg += "To: ";
	msg += destFolder;
	TheAlertMan.PostInfoAlert (msg, true, false, why); // verbose, not low priority
}

bool FolderMan::CopyMaterialize (char const * src, FilePath const & destFolder, 
                                 char const * filename, bool & driveNotReady, bool quiet)
{
    driveNotReady = false;
	if (File::CopyNoEx (src, destFolder.GetFilePath (filename)))
		return true;
		
	int err = Win::GetError ();
	Win::ClearError ();
	if (quiet)
	{
		bool exit = true;
		if (err == Win::NotReady)
		{
			driveNotReady = true;
		}
		else if	(err == Win::PathNotFound)
		{
			// Don't confirm, don't throw
			exit = !FolderMan::CreateFolder (destFolder.GetDir (), false, true);
		}
		if (exit)
			return false;
	}
	else
	{
		// verbose
		switch (err)
		{
		case Win::NotReady:
		{
			Out::PromptStyle style (Out::RetryCancel, Out::Retry, Out::Warning);
			Out::Answer answer = TheOutput.Prompt (
									"Drive not ready: Please insert a disk.",
									style);
			if (Out::Cancel == answer)
								  
			{
				driveNotReady = true;
				return false;
			}
			// Don't confirm, don't throw
			if (!FolderMan::CreateFolder (destFolder.GetDir (), false, true))
			{
				driveNotReady = true;
        		LastSysErr errMsg;
				DisplayError ("Create folder failed", errMsg.Text (), filename, destFolder.GetDir ());
				return false;
			}
		}
		break;
		case Win::BadNetPath:
		case Win::NetUnreachable:
		{
			std::string msg = destFolder.GetDir ();
			msg += " is not accessible.";
			std::string hint = "You can try again later by selecting \n\"Dispatch now\" from the Dispatcher's menu.";
			TheAlertMan.PostInfoAlert (msg, true, true, hint); // verbose, low priority
			return false;
		}
		break;
		case Win::PathNotFound:
		{
			// Don't confirm, don't throw
			if (!FolderMan::CreateFolder (destFolder.GetDir (), false, true))
			{
        		LastSysErr errMsg;
				DisplayError ("Create folder failed", errMsg.Text (), filename, destFolder.GetDir ());
				return false;
			}
		}
		break;
		default:
			{
				 
				SysMsg errmsg (err);
				std::string msg = "Cannot copy script:\n";
				msg += filename;
				msg += "\nto:\n";
				msg += destFolder.GetDir ();
				TheAlertMan.PostInfoAlert (msg, true, false, errmsg.Text ()); // verbose, not low priority
				return false;
			}
		};
	}
	// In some cases we try copying again
	if (File::CopyNoEx (src, destFolder.GetFilePath (filename)))
	{
		return true;
	}
	else
	{
		if (!quiet)
		{
			LastSysErr errMsg;
			DisplayError ("Copy unsuccessful", errMsg.Text (), filename, destFolder.GetDir ());
		}
		return false;
	}
}

bool FolderMan::CreateFolder (char const * path, bool needsConfirmation, bool quiet)
{
    // Count how many levels we have to create
    // If more then one ask the user for confirmation
    XFolderList folderList;
    FullPathSeq checkSeq (path);
	FilePath workPath (checkSeq.GetHead ());
    int depth = 0;
    for (; !checkSeq.AtEnd (); checkSeq.Advance ())
    {
        workPath.DirDown (checkSeq.GetSegment ().c_str ());
        if (!File::Exists (workPath.GetDir ()))
            depth++;
    }
    if (depth > 1 && needsConfirmation)
    {
        Msg info;
        info << "The path: '" << path << "'\n\ndoes not exist. Do you want to create it?";
		Out::Answer answer = TheOutput.Prompt (info.c_str ());
        if (answer == Out::No && !quiet)
            throw Win::Exception ();
        else if (answer != Out::Yes)
            return false;
    }

	if (depth > 0)
	{
		// Now materialize folder path
		FullPathSeq folderSeq (path);
		workPath.Change (folderSeq.GetHead ());
		for (; !folderSeq.AtEnd (); folderSeq.Advance ())
		{
			workPath.DirDown (folderSeq.GetSegment ().c_str ());
			char const * path = workPath.GetDir ();
			if (File::CreateNewFolder (path, quiet))
			{
			    folderList.RememberCreatedDir (workPath.GetDir ());
			}
			else
			{
				return false;
			}
		}
	}
    folderList.Commit ();
    return true;
}

void XFolderList::RememberCreatedDir (char const * fullPath)
{
    _created.push_back (fullPath);
}

static void QuietDelete (std::string const & path)
{
	ShellMan::Delete (0, path.c_str ());
}

XFolderList::~XFolderList ()
{
    if (!_commit)
        std::for_each (_created.begin (), _created.end (), &QuietDelete);
}

bool CopyFailedCtrl::OnInitDialog () throw (Win::Exception)
{
	_what.Init (GetWindow (), IDC_WHAT);
	_why.Init (GetWindow (), IDC_WHY);
	_script.Init (GetWindow (), IDC_SCRIPT);
	_dest.Init (GetWindow (), IDC_DEST);

	_what.SetText (_dlgData->_what.c_str ());
    _why.SetText (_dlgData->_why.c_str ());
    _script.SetString (_dlgData->_script.c_str ());
    _dest.SetString (_dlgData->_dest.c_str ());

    GetWindow ().SetForeground ();
	return true;
}

bool CopyFailedCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    switch (ctrlId)
    {
    case Out::OK:
        EndOk (); 
        return true;
    }
    return false;
}
