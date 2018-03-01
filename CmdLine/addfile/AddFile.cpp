//------------------------------------
//  (c) Reliable Software, 2000 - 2005
//------------------------------------

#include "precompiled.h"
#include "CmdArgs.h"
#include "SccErrorOut.h"
#include "SccProxy.h"
#include "SccOptions.h"
#include "CmdLineVersionLabel.h"

#include <File/File.h>
#include <Ex/WinEx.h>
#include <Ex/Error.h>

#include <iostream>

class AddSwitch : public StandardSwitch
{
public:
	AddSwitch ()
	{
		SetTypeHeader ();
		SetTypeSource ();
		SetTypeText ();
		SetTypeBinary ();
		SetDoCheckIn ();
		SetComment ();
		SetKeepCheckedOut ();
	}
};

int main (int count, char * args [])
{
	int retCode = 0;
	try
	{
		AddSwitch validSwitch;
		CmdArgs cmdArgs (count, args, validSwitch);
		if (cmdArgs.IsHelp () || cmdArgs.IsEmpty ())
		{
			std::cout << CMDLINE_VERSION_LABEL << ".\n\n";
			std::cout << "Add files to the project. Ignores files not present on disk.\n\n";
			std::cout << "addfile <options> <files>\n";
			std::cout << "options:\n";
			std::cout << "   -t:header -- header file\n";
			std::cout << "   -t:source -- source file\n";
			std::cout << "   -t:text   -- other text file\n";
			std::cout << "   -t:binary -- binary file\n";

			std::cout << "   -i -- check in added files\n";
			std::cout << "   -c:\"comment\" -- check-in comment\n";
			std::cout << "   -o -- when using -i, keep the files checked out\n";

			std::cout << "   -? -- display help\n";
		}
		else
		{
			CodeCoop::Proxy sccProxy (&SccErrorOut);
			CodeCoop::SccOptions options;
			if (!cmdArgs.IsDoCheckIn ())
				options.SetDontCheckIn ();
			else if (cmdArgs.IsKeepCheckedOut ())
				options.SetKeepCheckedOut ();
			if (cmdArgs.IsHeaderFile ())
				options.SetTypeHeader ();
			else if (cmdArgs.IsSourceFile ())
				options.SetTypeSource ();
			else if (cmdArgs.IsTextFile ())
				options.SetTypeText ();
			else if (cmdArgs.IsBinaryFile ())
				options.SetTypeBinary ();
			StringArray	paths;
			char const ** argPaths = cmdArgs.GetFilePaths ();
			for (unsigned i = 0; i < cmdArgs.Size (); ++i)
			{
				if (File::Exists (argPaths [i]))
					paths.push_back (argPaths [i]);
			}
			if (!sccProxy.AddFile (paths.size (), paths.get (), cmdArgs.GetComment (), options))
			{
				std::cerr << "addfile: Cannot add selected files." << std::endl;
				retCode = 1;
			}
		}
	}
	catch (Win::Exception e)
	{
		std::cerr << "addfile: " << e.GetMessage () << std::endl;
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
		std::cerr << "addfile: Unknown problem\n";
		retCode = 1;
	}
	return retCode;
}
