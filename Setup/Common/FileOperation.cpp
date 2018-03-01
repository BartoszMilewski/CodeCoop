//------------------------------------
//  (c) Reliable Software, 1999 - 2009
//------------------------------------

#include "precompiled.h"
#include "FileOperation.h"
#include "Proxy.h"
#include "OutputSink.h"
#include "SetupParams.h"
#include <Win/Message.h>
#include <File/File.h>
#include <File/Path.h>
#include <File/Dir.h>

namespace Setup
{
	void DeleteFiles (char const * const fileList [], FilePath const & from)
	{
		for (int i = 0; fileList [i] != 0; i++)
		{
			char const * coopFile = from.GetFilePath (fileList [i]);
			File::DeleteNoEx (coopFile);
		}
	}

	void DeleteFilesAndFolder (char const * const * files, FilePath const & folderPath)
	{
		Assert (!folderPath.IsDirStrEmpty ());
		for (int i = 0; files [i] != 0; i++)
		{
			File::DeleteNoEx (folderPath.GetFilePath (files [i]));
		}
		// Delete the folder if it is empty.
		//  ::RemoveDirectory only succeeds on empty directories
		::RemoveDirectory (folderPath.GetDir ());
		Win::ClearError ();
	}

	bool IsEmptyFolder (FilePath const & folder)
	{
		for (DirSeq dirSeq (folder.GetAllFilesPath ()); !dirSeq.AtEnd (); dirSeq.Advance ())
		{
			char const * subDir = dirSeq.GetName ();
			if (subDir [0] == '.' && subDir [1] == '.')
				continue;

			//	found a subfolder, so this one's not empty
			return false;
		}
		for (FileSeq fileSeq (folder.GetAllFilesPath ()); !fileSeq.AtEnd (); fileSeq.Advance ())
		{
			//	found a fild, so this one's not empty
			return false;
		}

		//	nothing found - we're empty
		return true;
	}

	void DeleteIntegratorFiles (FilePath const & fromPath)
	{
		Assert (!fromPath.IsDirStrEmpty ());
		//	Make sure IDE integrators are not in use
		// (which it might be if VS .Net is running)
		for (unsigned int i = 0; IdeIntegrators [i] != 0; ++i)
		{
			char const * integratorFile = IdeIntegrators [i];
			while (File::Exists (fromPath.GetFilePath (integratorFile)))
			{
				if (!File::DeleteNoEx (fromPath.GetFilePath (integratorFile)))
				{
					//	prompt user that SccDll is in use (Retry, Cancel)
					std::string question;
					question += "A Code Co-op file, ";
					question += integratorFile;
					question += ", is in use by some other application\r\n\r\n"
								"To continue installation, exit all applications\r\n"
								"that might be using this file (i.e. development tools),\r\n"
								"and then choose Retry.\r\n\r\n";
					question += "Also, make sure you have appropriate permissions\r\n"
								"to modify files in the Program Files directory";
					
					if (Out::Cancel == TheOutput.PromptModal (question.c_str (), Out::PromptStyle (Out::RetryCancel, Out::Retry, Out::Error)))
						throw Win::Exception ("Installation was canceled by the user.");
#if 0
					// Only for testing!
					std::string newName = "Old";
					newName += integratorFile;
					std::string newPath = fromPath.GetFilePath (newName);
					File::DeleteNoEx (newPath);
					File::Move (fromPath.GetFilePath (integratorFile), newPath.c_str ());
					break;
#endif
				}
			}
		}
	}

	void CloseRunningApp ()
	{
		// Co-op
		do
		{
			CoopProxy coop;
			if (coop.GetWin () == 0)
				break;

			coop.Kill ();
		} while (true);
		// Dispatcher
		DispatcherProxy dispatcher;
		dispatcher.Kill ();
		// Differ
		do
		{
			// first, try to close it
			Win::Dow::Handle currentDifferWin;
			{
				DifferProxy differ;
				if (differ.GetWin () == 0)
					break;
		
				currentDifferWin = differ.GetWin ();
				// it will close if there are no unsaved changes 
				differ.PostMsg (Win::CloseMessage ());
			}
			DifferProxy tryAgainDiffer;
			if (tryAgainDiffer.GetWin () == currentDifferWin)
			{
				// didn't close
				TheOutput.Display ("Code Co-op Installer has to close the Differ.\n"
								   "If you have any unsaved changes, please save them now.");
			}
		} while (true);
	}
}
