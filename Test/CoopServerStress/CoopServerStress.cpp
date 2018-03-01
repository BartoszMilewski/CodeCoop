//-----------------------------------
// (c) Reliable Software 2007
//-----------------------------------

#include "precompiled.h"

#include "PathRegistry.h"

#include <File/File.h>
#include <File/Path.h>
#include <sys/Process.h>

#include <iostream>

int main (int argc, char * argv [])
{
	FilePath installationFolder (Registry::GetProgramPath ());
	TmpPath tmpPath;

	if (argc != 2)
	{
		std::cerr << "Usage: coopserverstress <local project id>" << std::endl;
		return 1;
	}

	if (!File::Exists (installationFolder.GetFilePath ("coopcmd.exe")))
	{
		std::cerr << installationFolder.GetFilePath ("coopcmd.exe") << " not found!." << std::endl;
		return 1;
	}

	std::string cmdline;
	cmdline += "\"";
	cmdline += installationFolder.GetFilePath ("coopcmd.exe");
	cmdline += "\" -p:";
	cmdline += argv [1];
	cmdline += " -c:\"Help_SaveDiagnostics catalog:\"\"yes\"\" target:\"\"";
	cmdline += tmpPath.GetFilePath ("ATestDiag.txt");
	cmdline += "\"\"\"";

	while (true)
	{
		Win::ChildProcess coopCmd (cmdline);
		coopCmd.SetCurrentFolder (installationFolder.GetDir ());
		coopCmd.ShowMinimizedNotActive ();
		coopCmd.Create ();

		::Sleep (10000);	// 10 sec

		if (coopCmd.IsAlive ())
		{
			std::cerr << "CoopServerStress: after 10 s server still alive!" << std::endl;
		}
		File::DeleteNoEx (tmpPath.GetFilePath ("ATestDiag.txt"));
	}
	return 0;
}
