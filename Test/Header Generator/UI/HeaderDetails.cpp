// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "HeaderDetails.h"
#include "OutputSink.h"

HeaderDetails::HeaderDetails ()
: _wasCanceled (false),
  _toBeForwarded (false),
  _isDefect (false),
  _hasAddendums (false),
  _destFolder (PublicInbox)
{
	// generate script name
	// Revisit: temporary
	_scriptFilename = "script1.snc";
}


bool HeaderDetails::IsDestDataValid ()
{
	if (!_scriptFilename.empty ())
	{
		if (FilePath::IsValid (_scriptFilename.c_str ()))
		{
			if (_destFolder == HeaderDetails::UserDefined)
			{
				if (!_destPath.IsEmpty ())
				{
					if (FilePath::IsValid (_destPath.GetDir ()) && 
						FilePath::IsAbsolute (_destPath.GetDir ()))
						return true;
				}
			}
			else
				return true;
		}
	}
	return false;
}

void HeaderDetails::DisplayDestErrors ()
{
	if (_scriptFilename.empty ())
	{
		TheOutput.Display ("Script filename is not defined");
	}
	else if (!FilePath::IsValid (_scriptFilename.c_str ()))
	{
		TheOutput.Display ("Script filename contains illegal characters");
	}
	else if (_destPath.IsEmpty ())
	{
		TheOutput.Display ("Destination path is not defined");
	}
	else if (FilePath::IsValid (_destPath.GetDir ()))
	{
		TheOutput.Display ("Destination path contains illegal characters");
	}
	else
	{
		TheOutput.Display ("Destination path must be a full path");
	}
}
