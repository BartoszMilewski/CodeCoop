//---------------------------------------------
// (c) Reliable Software 2001 -- 2003
//---------------------------------------------

#include "precompiled.h"
#include "Commander.h"
#include "Model.h"
#include "PathSequencer.h"
#include "FolderContents.h"
#include "AddFilesDlg.h"
#include "ProgressMeter.h"
#include "Prompter.h"
#include "resource.h"

#include <Win/MsgLoop.h>
#include <Com/Shell.h>
#include <Ex/WinEx.h>
#include <File/File.h>

#include <StringOp.h>

Commander::Commander (Model & model, Win::MessagePrepro * msgPrepro, Win::Dow::Handle hwnd)
	: _model (model),
	  _hwnd (hwnd),
	  _msgPrepro (msgPrepro)
{
}

void Commander::Test_Run ()
{
    ShellMan::VirtualDesktopFolder root;
    ShellMan::FolderBrowser folder (_hwnd, root, "Select folder to compress");
    if (folder.IsOK ())
    {
		ProgressMeterDlg progress ("Scanning folder contents", _hwnd, *_msgPrepro);
		// Scan folder contents
		std::vector<std::string> files;
		NocaseSet emptyFilter;
		FolderContents contents (folder.GetPath (), emptyFilter, progress);
		if (!contents.IsEmpty ())
		{
			contents.Sort (progress);
			progress.Close ();
			AddFilesCtrl ctrl (&contents);
			if (ThePrompter.GetData (ctrl))
			{
				unsigned int fileCount = 0;
				std::vector<std::string> extensions;
				contents.GetExtensionList (extensions);
				for (std::vector<std::string>::const_iterator extIter = extensions.begin ();
					extIter != extensions.end ();
					++extIter)
				{
					FolderContents::FileSeq fileSeq = contents.GetFileList (*extIter);
					for ( ; !fileSeq.AtEnd (); fileSeq.Advance ())
					{
						if (fileSeq.IsSelected ())
						{
							fileCount++;
						}
					}
				}
				ProgressMeterDlg compresionProgress ("Compressing selected files", _hwnd, *_msgPrepro);
				std::string packedFiles;
				_model.Compress (contents, fileCount, packedFiles, compresionProgress);
				ProgressMeterDlg verifyProgress ("Compression verification", _hwnd, *_msgPrepro);
				_model.VerifyCompresion (contents, fileCount, packedFiles, verifyProgress);
				File::Delete (packedFiles.c_str ());
			}
		}
    }
}

void Commander::Test_Exit ()
{
	throw Win::ExitException (0);
}
