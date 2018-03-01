//------------------------------------------------
// (c) Reliable Software 2002
// -----------------------------------------------

#include "precompiled.h"
#include "AppHelp.h"
#include "OutputSink.h"
#include "PathRegistry.h"

#include <Com/Shell.h>
#include <File/File.h>
#include <File/Path.h>
#include <Dbg/Out.h>
#include <Win/Help.h>

namespace AppHelp
{
	void Display(char const *topic, char const *content, Win::Dow::Handle winParent)
	{
        std::string pgmPath = Registry::GetProgramPath ();
		if (pgmPath.empty ())
		{
			throw Win::Exception ("Cannot locate help file");
		}

		FilePath installPath (pgmPath);

		//	If we there is a .chm file installed, use that
		char const * hlpFilePath = installPath.GetFilePath (AppHelp::CodeCoopHelpFile);

		if (File::Exists(hlpFilePath))
		{
			Help::Module helpModule(hlpFilePath);

			if (helpModule.DisplayTopic(topic, winParent))
			{
				return;
			}
		}

		//	No .chm file found, so look for the old style html files
		hlpFilePath = installPath.GetFilePath(topic);
		
		int errCode = ShellMan::Open (winParent, hlpFilePath);
	    if (errCode != -1)
	    {
			std::string msg = ShellMan::HtmlOpenError (errCode, content, hlpFilePath);
			TheOutput.Display (msg.c_str (), Out::Error);
		}
	}
}