//----------------------------------
// (c) Reliable Software 2007 - 2008
//----------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "CmdLineVersionLabel.h"
#include "PathRegistry.h"
#include "GlobalFileNames.h"
#include "BackupFileName.h"

#include <Sys/Process.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

class BackupSwitch : public StandardSwitch
{
public:
	BackupSwitch ()
	{
		SetOverwrite ();
		SetFtpServer ();
		SetFtpFolder ();
		SetFtpUser ();
		SetFtpPassword ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		BackupSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || (cmdArgs.IsEmpty () && !cmdArgs.IsFtpFolder ()))
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Start creating Code Co-op backup archive.\n\n"; 
			std::cout << "Local or LAN backup:\n";
			std::cout << "    startcoopbackup <options> <LAN or local target path>\n";
			std::cout << "    options:\n";
			std::cout << "       -o -- overwrite existing backup archive\n";
			std::cout << "       -? -- display help\n";
			std::cout << "FTP backup:\n";
			std::cout << "    startcoopbackup <options> -f:<target path on the server>\n";
			std::cout << "    options:\n";
			std::cout << "       -o -- overwrite existing backup archive\n";
			std::cout << "       -? -- display help\n";
			std::cout << "       -s:<ftp server>\n";
			std::cout << "       -u:<user name>\n";
			std::cout << "       -p:<password>\n";
		}
		else
		{
			BackupFileName backupName;
			// Tools_CreateBackup overwrite:"yes | no" target:"<backup archive path>" server:"<ftp server>" user:"<ftp user>" password:"<ftp password>"
			std::string backupCmd (CoopExeName);// Executable name will be stripped from the
												// command line passed to the co-op.exe
			backupCmd += " -GUI -Tools_CreateBackup ";
			backupCmd += "overwrite:\"";
			backupCmd += (cmdArgs.IsOverwrite () ? "yes" : "no");
			backupCmd += "\" ";
			if (cmdArgs.IsFtpServer ())
			{
				backupCmd += "target:\"";
				backupCmd += cmdArgs.GetFtpFolder ();
				backupCmd += "\\";
				backupCmd += backupName.Get ();
				backupCmd += "\"";
				backupCmd += " server:\"";
				backupCmd += cmdArgs.GetFtpServer ();
				backupCmd += "\"";
				if (cmdArgs.IsFtpUser ())
				{
					backupCmd += " user:\"";
					backupCmd += cmdArgs.GetFtpUser ();
					backupCmd += "\"";
				}
				if (cmdArgs.IsFtpPassword ())
				{
					backupCmd += " password:\"";
					backupCmd += cmdArgs.GetFtpPassword ();
					backupCmd += "\"";
				}
			}
			else
			{
				backupCmd += "target:\"";
				char const ** filePaths = cmdArgs.GetFilePaths ();
				backupCmd += filePaths [0];
				backupCmd += "\\";
				backupCmd += backupName.Get ();
				backupCmd += "\"";
			}

			Win::ChildProcess coop (backupCmd.c_str (), false);	// Don't inherit parent's handles
			FilePath installPath (Registry::GetProgramPath ());
			coop.SetAppName (installPath.GetFilePath (CoopExeName));
			coop.ShowNormal ();
			coop.Create ();
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "startcoopbackup: " << e.GetMessage () << std::endl;
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
		std::cerr << "startcoopbackup: Unknown problem" << std::endl;
		retCode = 1;
	}
	return retCode;
}
