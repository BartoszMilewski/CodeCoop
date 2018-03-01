//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "CopyRequest.h"
#include "OutputSink.h"

#include <File/Path.h>

std::string FileCopyRequest::GetTargetFolder () const
{
	if (IsInternet ())
		return std::string ();
	else
		return _localFolder;
}

bool FileCopyRequest::IsValid (bool testFtpWrite)
{
	if (!IsInternet () && !IsMyComputer () && !IsLAN ())
		return false;

	if (IsInternet ())
	{
		_ftpSite.reset (new Ftp::Site (_ftpLogin, _ftpLogin.GetFolder ()));
		return _ftpSite->TestConnection (testFtpWrite);
	}

	Assert (IsMyComputer () || IsLAN ());

	return !_localFolder.empty ()					&&
			FilePath::IsValid (_localFolder)		&&
			FilePath::IsAbsolute (_localFolder)		&&
			FilePath::HasValidDriveLetter (_localFolder);
}

void FileCopyRequest::DisplayErrors (Win::Dow::Handle owner) const
{
	if (IsInternet ())
	{
		Assert (_ftpSite.get () != 0);
		_ftpSite->DisplayErrors ();
		return;
	}

	Assert (IsMyComputer () || IsLAN ());

	if (_localFolder.empty ())
	{
		TheOutput.Display ("Please, specify a folder path.", Out::Information, owner);
	}
	else if (!FilePath::IsValid (_localFolder))
	{
		std::string info ("The following path contains illegal characters:\n\n");
		info += _localFolder;
		TheOutput.Display (info.c_str (), Out::Information, owner);
	}
	else if (!FilePath::IsAbsolute (_localFolder))
	{
		TheOutput.Display ("Please, specify absolute path.");
	}
	else
	{
		Assert (!FilePath::HasValidDriveLetter (_localFolder));
		std::string info ("The following path contains illegal drive:\n\n");
		info += _localFolder;
		TheOutput.Display (info.c_str (), Out::Information, owner);
	}
}
