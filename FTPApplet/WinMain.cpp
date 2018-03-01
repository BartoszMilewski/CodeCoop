//----------------------------------
// (c) Reliable Software 2007 - 2008
//----------------------------------

#include "precompiled.h"
#include "PathRegistry.h"
#include "OutputSink.h"
#include "FtpSite.h"
#include "CmdLineArgs.h"

#include <Win/MsgLoop.h>
#include <Ctrl/ProgressDialog.h>
#include <Ctrl/MultiProgressDialog.h>
#include <File/File.h>
#include <File/Dir.h>
#include <Net/FtpUploader.h>
#include <Net/FtpDownloader.h>
#include <Com/Shell.h>

static char const FtpAppletCaption [] = "Code Co-op FTP";

void StartDownload (Ftp::Login const & login,
					std::string const & sourcePath,
					std::string const & targetPath,
					bool isSingleFile);
void StartUpload (Ftp::Login const & login,
				  std::string const & sourcePath,
				  std::string const & targetPath,
				  bool isSingleFile,
				  bool doCleanup);

int PASCAL WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdParam, int cmdShow)
{
	CmdLineArgs cmdLineArgs (cmdParam);
	if (!cmdLineArgs.IsDownload () && !cmdLineArgs.IsUpload ())
		return 1;

	int result = 0;
	try
	{
		TheOutput.Init (FtpAppletCaption, Registry::GetLogsFolder ());

		Ftp::Login login;
		login.SetServer (cmdLineArgs.GetServer ());
		login.SetUser (cmdLineArgs.GetUser ());
		login.SetPassword (cmdLineArgs.GetPassword ());

		if (cmdLineArgs.IsDownload ())
			StartDownload (login,
						   cmdLineArgs.GetRemotePath (),
						   cmdLineArgs.GetLocalPath (),
						   cmdLineArgs.IsFileDownload ());
		else
			StartUpload (login,
						 cmdLineArgs.GetLocalPath (),
						 cmdLineArgs.GetRemotePath (),
						 cmdLineArgs.IsFileUpload (),
						 !cmdLineArgs.IsNoCleanup());
	}
	catch (Win::Exception e)
	{
		if (e.GetMessage () != 0)
		{
			TheOutput.Display (e);
			result = 1;
		}
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
		result = 1;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
		result = 1;
	}

	return result;
}

void StartDownload (Ftp::Login const & login,
					std::string const & sourcePath,
					std::string const & targetPath,
					bool isSingleFile)
{
	Win::MessagePrepro msgPrepro;
	Progress::MeterDialog progressDialog (FtpAppletCaption,
										  0,	// Display on the desktop
										  msgPrepro);
	std::string caption;
	if (isSingleFile)
		caption = "Downloading file ";
	else
		caption = "Downloading files from the folder ";
	caption += login.GetServer ();
	caption += "/";
	caption += sourcePath;
	caption += " to ";
	caption += targetPath;
	progressDialog.SetCaption (caption);

	if (isSingleFile)
	{
		File::MaterializePath (targetPath.c_str ());
		PathSplitter splitter (sourcePath);
		std::string folder = splitter.GetDir ();

		Ftp::Site ftpSite (login);
		ftpSite.SetRemoteDirectory (folder);
		std::string fileName (splitter.GetFileName ());
		fileName += splitter.GetExtension ();
		if (ftpSite.Download (fileName, targetPath, progressDialog.GetProgressMeter ()))
		{
			caption += " succeeded.";
			TheOutput.Display (caption.c_str ());
		}
		else
		{
			ftpSite.DisplayErrors ();
		}
	}
	else
	{
		Ftp::DownloadTraversal traversal (sourcePath,
										  targetPath,
										  login,
										  progressDialog.GetProgressMeter ());
		if (traversal.CopyFiles ())
			caption += " succeeded.";
		else
			caption += " failed.";

		TheOutput.Display (caption.c_str ());
	}
}

void StartUpload (Ftp::Login const & login,
				  std::string const & sourcePath,
				  std::string const & targetPath,
				  bool isSingleFile,
				  bool doCleanup)
{
	Win::MessagePrepro msgPrepro;
	std::string caption;
	if (isSingleFile)
		caption = "Uploading file ";
	else
		caption = "Uploading files from the folder ";
	caption += sourcePath;
	caption += " to ";
	caption += login.GetServer ();
	caption += "/";
	caption += targetPath;

	if (isSingleFile)
	{
		if (!File::Exists (sourcePath))
			throw Win::InternalException ("File not found.", sourcePath.c_str ());

		// Move inside StartUpload/Download
		Progress::MeterDialog progressDialog (FtpAppletCaption,
											  0,	// Display on the desktop
											  msgPrepro);
		progressDialog.SetCaption (caption.c_str ());
		PathSplitter splitter (sourcePath);
		std::string fileName (splitter.GetFileName ());
		fileName += splitter.GetExtension ();

		Ftp::Site ftpSite (login);
		ftpSite.SetRemoteDirectory (targetPath);
		if (ftpSite.Upload (sourcePath, fileName, progressDialog.GetProgressMeter ()))
		{
			if (doCleanup)
				File::DeleteNoEx (sourcePath);
			caption += " succeeded.";
			TheOutput.Display (caption.c_str ());
		}
		else
		{
			if (doCleanup)
			{
				// Upload failed - move source file to the Desktop
				FilePath userDesktopPath;
				ShellMan::VirtualDesktopFolder userDesktop;
				userDesktop.GetPath (userDesktopPath);
				File::Move (sourcePath.c_str (),
							userDesktopPath.GetFilePath (fileName));
			}

			ftpSite.DisplayErrors ();

			if (doCleanup)
			{
				std::string msg = "The file has been saved to your desktop.\n\n";
				msg += fileName;
				TheOutput.Display (msg.c_str ());
			}
		}
	}
	else
	{
		// Multiple file upload
		if (!File::Exists (sourcePath))
			throw Win::InternalException ("Source directory not found.", sourcePath.c_str ());

		Progress::MultiMeterDialog progressDialog (FtpAppletCaption,
												   0,	// Display on the desktop
												   msgPrepro);
		progressDialog.SetCaption (caption);

		try
		{
			Ftp::UploadTraversal traversal (sourcePath,
											targetPath,
											login,
											progressDialog.GetOverallMeter (),
											progressDialog.GetSpecificMeter ());
			traversal.MaterializeFoldersAtTarget ();
			traversal.CopyFiles ();
			caption += " succeeded.";
		}
		catch (Win::Exception ex)
		{
			if (ex.GetMessage () == 0)
			{
				caption += " was canceled by the user.";
			}
			else
			{
				caption += " failed.\n\n";
				caption += Out::Sink::FormatExceptionMsg (ex);
			}
			
		}
		catch ( ... )
		{
			caption += " failed.";
		}
		TheOutput.Display (caption.c_str ());
		try
		{
			File::DeleteTree (sourcePath);
		}
		catch (...)
		{}
	}
}
