//----------------------------------
//  (c) Reliable Software, 1997-2002
//----------------------------------

#include "precompiled.h"
#include "AppInfo.h"
#include "PathRegistry.h"
#include "OutputSink.h"

#include <Ex/WinEx.h>
#include <File/File.h>

AppInformation TheAppInfo;

char const BadSetupError [] =	"Program path is not set.\n"
								"Please reinstall Code Co-op.";

void AppInformation::InitPaths ()
{
	try
	{
		std::string pgmPath = Registry::GetProgramPath ();
		if (!pgmPath.empty ())
		{
			if (File::Exists (pgmPath.c_str ()))
			{
				_pgmPath.Change (pgmPath);
			}
			else
			{
				std::string msg ("Setup path is not accessible: ");
				msg += pgmPath.c_str ();
				TheOutput.Display (msg.c_str (), Out::Error);
			}
		}
		else
			TheOutput.Display (BadSetupError, Out::Error);
	}
    catch (Win::Exception e)
    {
        TheOutput.Display (e);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Initializing General Program Information -- Unknown Error", Out::Error); 
    }
}

bool AppInformation::IsTemporaryUpdate () const
{
	return File::Exists (_pgmPath.GetFilePath (TempVersionMarker));
}
