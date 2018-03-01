//---------------------------
// (c) Reliable Software 2010
//---------------------------
#include "precompiled.h"
#include "PrepareFiles.h"
#include "AppInfo.h"
#include <Net/Ftp.h>
#include <File/File.h>
#include <Sys/Process.h>

Uploader::Uploader(FilePath & coopRoot, FilePath & webRoot, Ftp::Login const & login)
: _coopRoot(coopRoot),
  _webRoot(webRoot),
  _setupPath(coopRoot),
  _uploadPath(coopRoot),
  _login(login)
{
	_setupPath.DirDown("Setup");
	_uploadPath.DirDown("Setup\\Upload");

	AppInformation info;
	info.InitPaths();

	_ftpAppPath = info.GetFtpAppletPath ();
	if (!File::Exists (_ftpAppPath))
		throw Win::InternalException ("Missing Code Co-op installation file. Run setup program again.",
									  _ftpAppPath.c_str ());
}

void Uploader::PrepareFiles()
{
	for (int i = 0; i < productCount; ++i)
	{
		ProductId id = ProductNames::prodIds[i];
		std::string srcPath = _uploadPath.GetFilePath(TheProductNames.Get(id) + ".exe");
		std::string tgtPath = _uploadPath.GetFilePath(TheProductNames.GetV(id) + ".exe");
		File::Copy(srcPath.c_str(), tgtPath.c_str());
	}
	File::Copy(_setupPath.GetFilePath("CoopVersion.xml"), _uploadPath.GetFilePath("CoopVersion.xml"));
	// Files from web project
	File::Copy(_webRoot.GetFilePath("co_op\\Bulletin51.html"), _uploadPath.GetFilePath("Bulletin51.html"));
	File::Copy(_webRoot.GetFilePath("co_op\\download.html"), _uploadPath.GetFilePath("download.html"));
	File::Copy(_webRoot.GetFilePath("co_op\\Help\\ReleaseNotes.html"), _uploadPath.GetFilePath("ReleaseNotes.html"));
}

/*
mput CoopVersion.xml co-op.exe co-op51d.exe Co-opLite.exe Co-opLite51d.exe cmdline.exe cmdline51d.exe
cd www
mput CoopVersion.xml co-op51d.exe Co-opLite51d.exe
cd co_op
mput Bulletin51.html download.html
cd Help
put ReleaseNotes.html
*/

void Uploader::Upload()
{
	// ftp site
	UploadFile(_uploadPath.GetFilePath("CoopVersion.xml"), "");
	for (int i = 0; i < productCount; ++i)
	{
		ProductId id = ProductNames::prodIds[i];
		UploadFile(_uploadPath.GetFilePath(TheProductNames.Get(id) + ".exe"), "");
		UploadFile(_uploadPath.GetFilePath(TheProductNames.GetV(id) + ".exe"), "");
	}

	// web site
	UploadFile(_uploadPath.GetFilePath("CoopVersion.xml"), "www");
	UploadFile(_uploadPath.GetFilePath(TheProductNames.GetV(pro) + ".exe"), "www");
	UploadFile(_uploadPath.GetFilePath(TheProductNames.GetV(lite) + ".exe"), "www");

	UploadFile(_uploadPath.GetFilePath("Bulletin51.html"), "www/co_op");
	UploadFile(_uploadPath.GetFilePath("download.html"), "www/co_op");
	UploadFile(_uploadPath.GetFilePath("ReleaseNotes.html"), "www/co_op/help");
}

void Uploader::UploadFile(std::string const & sourcePath, std::string const & targetPath)
{
	std::string cmdLine ("\"");
	cmdLine += _ftpAppPath;
	cmdLine += "\" ";
	cmdLine += "fileupload \"";
	cmdLine += sourcePath;
	cmdLine += "\" \"";
	cmdLine += targetPath;
	cmdLine += "\" ";
	cmdLine += _login.GetServer ();
	cmdLine += " \"";
	cmdLine += _login.GetUser ();
	cmdLine += "\" \"";
	cmdLine += _login.GetPassword ();
	cmdLine += "\" noCleanup";


	Win::ChildProcess ftpApplet (cmdLine);
	ftpApplet.SetAppName (_ftpAppPath);
	ftpApplet.ShowNormal ();
	if (!ftpApplet.Create ())
		throw Win::Exception ("Cannot start the FTP program.", _ftpAppPath.c_str ());
	ftpApplet.WaitForDeath(60 * 1000);
}
