//---------------------------
// (c) Reliable Software 2010
//---------------------------

#include "precompiled.h"
#include "OpenSaveFileData.h"
#include "Registry.h"
#include "PathFind.h"
#include "TestFile.h"
#include "OutputSink.h"
#include "AppInfo.h"

FileRequest::FileRequest (std::string const & typeValueName,
						  std::string const & ftpSiteValueName,
						  std::string const & folderValueName)
	: _typeValueName (typeValueName),
	  _ftpSiteValueName (ftpSiteValueName),
	  _folderValueName (folderValueName),
	  _quiet (false)
{
	Registry::UserPreferences prefs;

	Ftp::SmartLogin & login = _copyRequest.GetFtpLogin ();
	login.RetrieveUserPrefs (_ftpSiteValueName);
	_copyRequest.SetLocalFolder (prefs.GetFilePath (_folderValueName));
	std::string recentType = prefs.GetOption (_typeValueName);
	if (recentType == "internet")
		_copyRequest.SetInternet ();
	else if (recentType == "file")
		_copyRequest.SetLAN ();
}

void FileRequest::ReadNamedValues (NamedValues const & source)
{
	std::string target (source.GetValue ("target"));
	if (target.empty ())
		return;

	PathSplitter splitter (target);
	std::string path (splitter.GetDrive ());
	path += splitter.GetDir ();
	std::string server (source.GetValue ("server"));
	if (TheAppInfo.IsFtpSupportEnabled () && !server.empty ())
	{
		SetInternet ();
		Ftp::SmartLogin & ftpLogin = _copyRequest.GetFtpLogin ();
		ftpLogin.SetServer (server);
		ftpLogin.SetFolder (path);
		ftpLogin.SetUser (source.GetValue ("user"));
		ftpLogin.SetPassword (source.GetValue ("password"));
	}
	else
	{
		SetPath (path);
		if (FilePath::IsNetwork (path))
			SetLAN ();
		else
			SetMyComputer ();
	}

	std::string filename (splitter.GetFileName ());
	filename += splitter.GetExtension ();
	SetFileName (filename);
	std::string overwrite (source.GetValue ("overwrite"));
	SetOverwriteExisting (overwrite == "yes");
}


bool FileRequest::IsValid ()
{
	if (_fileName.empty ())
	{
		_errors.set (NoFileName, true);
		return false;
	}
	else if (_copyRequest.IsValid (!IsReadRequest ()))	// Test write access when creating file
	{
		if (IsStoreOnMyComputer () || IsStoreOnLAN ())
		{
			FilePath targetPath (_copyRequest.GetLocalFolder ());
			if (IsReadRequest ())
			{
				// Check if archive exist
				if (!File::Exists (targetPath.GetFilePath (_fileName)))
				{
					_errors.set (FileDoesntExist, true);
					return false;
				}
			}
			else
			{
				// Check if we have access to the target folder
				try
				{
					// Check if we can materialize target path on computer or LAN
					if (!File::Exists (targetPath.GetDir ()))
						PathFinder::MaterializeFolderPath (targetPath.GetDir ());

					// Can we create a file at target?
					LocalTestFile testFile (targetPath.GetFilePath (_fileName));
				}
				catch (Win::Exception e)
				{
					_errors.set (ExceptionThrown, true);
					_exceptionMsg = Out::Sink::FormatExceptionMsg (e);
					return false;
				}
				catch ( ... )
				{
					_errors.set (ExceptionThrown, true);
					_exceptionMsg = "Unknown exception during target path creation.";
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

void FileRequest::DisplayErrors (Win::Dow::Handle owner)
{
	if (!_copyRequest.IsMyComputer () && !_copyRequest.IsLAN () && !_copyRequest.IsInternet ())
	{
		std::string info ("Please, specify where to store the ");
		info += GetFileDescription ();
		info += '.';
		TheOutput.Display (info.c_str (), Out::Information, owner);
	}
	else if (_errors.test (NoFileName))
	{
		std::string info ("Please, specify the file ");
		info += GetFileDescription ();
		info += " name.";
		TheOutput.Display (info.c_str (), Out::Information, owner);
		_errors.set (NoFileName, false);
	}
	else if (_errors.test (FileDoesntExist))
	{
		std::string info ("The following ");
		info += GetFileDescription ();
		info += " doesn't exist:\n\n";
		FilePath targetPath (_copyRequest.GetLocalFolder ());
		info += targetPath.GetFilePath (_fileName);
		TheOutput.Display (info.c_str (), Out::Information, owner);
		_errors.set (FileDoesntExist, false);
	}
	else if (_errors.test (ExceptionThrown))
	{
		TheOutput.Display (_exceptionMsg.c_str (), Out::Information, owner);
		_errors.set (ExceptionThrown, false);
	}
	else if ((_copyRequest.IsMyComputer () || _copyRequest.IsLAN ()) &&
			  _copyRequest.GetLocalFolder ().length () == 0)
	{
		TheOutput.Display ("Please, specify a target folder path.", Out::Information, owner);
	}
	else
	{
		_copyRequest.DisplayErrors (owner);
	}
}

void FileRequest::RememberUserPrefs () const
{
	Registry::UserPreferences prefs;
	if (IsStoreOnInternet ())
	{
		prefs.SaveOption (_typeValueName, "internet");
		Ftp::SmartLogin const & login = _copyRequest.GetFtpLogin ();
		login.RememberUserPrefs (_ftpSiteValueName);
	}
	else
	{
		prefs.SaveOption (_typeValueName, "file");
		prefs.SaveFilePath (_folderValueName,
							_copyRequest.GetLocalFolder ());
	}
}

