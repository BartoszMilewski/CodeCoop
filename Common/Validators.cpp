// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "Validators.h"
#include "ScriptProcessorConfig.h"
#include "OutputSink.h"
#include <StringOp.h>
#include <File/File.h>

void UpdatePeriodValidator::DisplayError () const
{
	std::string info ("Please specify the check period between ");
	info += ToString (MinUpdatePeriod);
	info += " and ";
	info += ToString (MaxUpdatePeriod);
	info += " days.";
	TheOutput.Display (info.c_str ());
}

bool ScriptProcessorValidator::IsValid () const
{
	std::string preproCommand  = _cfg.GetPreproCommand ();
	std::string preproResult   = _cfg.GetPreproResult ();
	std::string postproCommand = _cfg.GetPostproCommand ();
	std::string postproExt     = _cfg.GetPostproExt ();
	bool preproNeedsProjName   = _cfg.PreproNeedsProjName ();
	// check prepro command: either both, command and result set, or neither
	if (preproCommand.empty ())
	{
		if (!preproResult.empty ())
			return false;
	}
	else if (preproResult.empty ()) // check prepro result
	{
		return false;
	}
	else
	{
		// check prepro resulting file syntax
		if (File::IsValidName (preproResult.c_str ()))
		{
			PathSplitter splitter (preproResult.c_str ());
			if (strlen (splitter.GetExtension ()) == 0)
				return false;
		}
		else
		{
			return false;
		}
	}

	// check postpro command: either both, command and extension set, or neither
	// check postpro extension
	if (postproExt.empty ())
	{
		if (!postproCommand.empty ())
			return false;
	}
	else if (postproCommand.empty ())  // check postpro command
	{
		return false;
	}
	else
	{
		// check postpro extension syntax
		if (!File::IsValidExtension (postproExt.c_str ()))
			return false;
	}
	 return true;
}

void ScriptProcessorValidator::DisplayErrors () const
{
	std::string preproCommand  = _cfg.GetPreproCommand ();
	std::string preproResult   = _cfg.GetPreproResult ();
	std::string postproCommand = _cfg.GetPostproCommand ();
	std::string postproExt     = _cfg.GetPostproExt ();
	bool preproNeedsProjName   = _cfg.PreproNeedsProjName ();
	// check prepro command: either both, command and result set, or neither
	if (preproCommand.empty ())
	{
		if (!preproResult.empty ())
		{
			TheOutput.Display ("Please specify the program you want to run"
				" before distributing scripts");
		}
	}
	else if (preproResult.empty ()) // check prepro result
	{
		TheOutput.Display ("Please specify the resulting filename produced by the program you want to run"
			" before distributing scripts");
	}
	else
	{
		// check prepro resulting file syntax
		if (File::IsValidName (preproResult.c_str ()))
		{
			PathSplitter splitter (preproResult.c_str ());
			if (strlen (splitter.GetExtension ()) == 0)
			{
				TheOutput.Display ("The specified pre-process result file name has no extension. "
					"Some email programs will add an extension to your file "
					"causing potential problems with unpacking your changes "
					"on the recipient's end. "
					"Please specify an extension.");
			}
		}
		else
		{
			TheOutput.Display ("The specified preprocessing result filename contains invalid characters. "
				"Please specify a correct filename.");
		}
	}

	// check postpro command: either both, command and extension set, or neither
	// check postpro extension
	if (postproExt.empty ())
	{
		if (!postproCommand.empty ())
		{
			TheOutput.Display ("Please specify the attachment extension you want to associate with the program"
				" run after receiving \"Code Co-op Synch Scripts\" email");
		}
	}
	else if (postproCommand.empty ())  // check postpro command
	{
		TheOutput.Display ("Please specify the program you want to run"
			" after receiving \"Code Co-op Synch Scripts\" email");
	}
	else
	{
		// check postpro extension syntax
		if (!File::IsValidExtension (postproExt.c_str ()))
		{
			TheOutput.Display ("The specified attachment extension contains invalid characters. "
				"Please specify a valid extension.");
		}
	}
}
