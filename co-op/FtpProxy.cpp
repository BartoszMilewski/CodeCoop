//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "AppInfo.h"

#include <Net/Ftp.h>
#include <Sys/Process.h>
#include <Ex/WinEx.h>

void ExecuteFtpUpload (std::string const & sourcePath,
					   std::string const & targetPath,
					   Ftp::Login const & login)
{
	std::string appPath (TheAppInfo.GetFtpAppletPath ());
	if (!File::Exists (appPath))
		throw Win::InternalException ("Missing Code Co-op installation file. Run setup program again.",
									  appPath.c_str ());

	std::string cmdLine ("\"");
	cmdLine += appPath;
	cmdLine += "\" ";
	if (File::IsFolder (sourcePath.c_str ()))
		cmdLine += "folderupload \"";
	else
		cmdLine += "fileupload \"";
	cmdLine += sourcePath;
	cmdLine += "\" \"";
	cmdLine += targetPath;
	cmdLine += "\" ";
	cmdLine += login.GetServer ();
	if (!login.IsAnonymous ())
	{
		cmdLine += " \"";
		cmdLine += login.GetUser ();
		cmdLine += "\" \"";
		cmdLine += login.GetPassword ();
		cmdLine += '"';
	}

	Win::ChildProcess ftpApplet (cmdLine);
	ftpApplet.SetAppName (appPath);
	ftpApplet.ShowNormal ();
	if (!ftpApplet.Create ())
		throw Win::Exception ("Cannot start the FTP program.", appPath.c_str ());
}

