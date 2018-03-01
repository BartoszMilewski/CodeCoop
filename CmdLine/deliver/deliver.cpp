//-----------------------------------------
//  (c) Reliable Software, 2000 -- 2003
//-----------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "Catalog.h"
#include "CmdLineVersionLabel.h"

#include <Ex/WinEx.h>
#include <Ex/Error.h>
#include <File/Path.h>
#include <File/Dir.h>
#include <Com/Shell.h>

#include <LightString.h>

#include <iostream>

static unsigned int ExpandWildCard (FilePath & folder, std::string & sourceFileList);

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		StandardSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch, true);	// Expecting only paths not file names
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Copy script files to the Public Inbox, so Dispatcher can deliver\n";
			std::cout << "them to the appropriate Code Co-op project.\n\n";
			std::cout << "deliver <script files>\n";
			std::cout << "if no <script files> specified -- deliver script files from the current folder.\n";
		}
		else
		{
			std::string sourceFileList;
			unsigned int fileCount = 0;
			if (cmdArgs.IsEmpty ())
			{
				CurrentFolder curFolder;
				fileCount = ExpandWildCard (curFolder, sourceFileList);
			}
			else
			{
				char const ** files = cmdArgs.GetFilePaths ();
				for (unsigned int i = 0; i < cmdArgs.Size (); ++i)
				{
					PathSplitter splitter (files [i]);
					std::string fname (splitter.GetFileName ());
					if (fname == "*" || fname.empty ())
					{
						std::string path (splitter.GetDrive ());
						path += splitter.GetDir ();
						FilePath folder (path);
						fileCount += ExpandWildCard (folder, sourceFileList);
					}
					else
					{
						sourceFileList += files [i];
						sourceFileList += '\0';
						fileCount++;
					}
				}
			}
			if (!sourceFileList.empty ())
			{
				sourceFileList += '\0';
				Catalog catalog;
				FilePath publicInbox = catalog.GetPublicInboxDir ();
				std::string targetPath (publicInbox.GetDir ());
				targetPath += '\0';
				ShellMan::CopyFiles (0,
									 sourceFileList.c_str (),
									 targetPath.c_str (),
									 ShellMan::UiNoConfirmation);
			}
			if (fileCount > 0)
				std::cout << fileCount << " script file(s) copied\n";
			else
				std::cout << "No script files copied\n";
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "Deliver: " << e.GetMessage () << std::endl;
		SysMsg msg (e.GetError ());
		if (msg)
			std::cerr << "System tells us: " << msg.Text ();
		std::string objectName (e.GetObjectName ());
		if (!objectName.empty ())
			std::cerr << "    " << objectName << std::endl;
		retCode = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		std::cerr << "Deliver: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}

static unsigned int ExpandWildCard (FilePath & folder, std::string & sourceFileList)
{
	unsigned int fileCount = 0;
	for (FileSeq seq (folder.GetFilePath ("*.snc")); !seq.AtEnd (); seq.Advance ())
	{
		sourceFileList += folder.GetFilePath (seq.GetName ());
		sourceFileList += '\0';
		fileCount++;
	}
	return fileCount;
}
