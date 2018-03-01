//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include "precompiled.h"
#include "CoopCmdLine.h"

CmdLine::CmdLine (std::string const & cmdName, FileListClassifier::ProjectFiles const * files)
{
	_line += " -";
	_line += cmdName;
	if (files == 0)
		return;
	// Don't attach file name list to commands starting with "All"
	std::string::size_type pos = cmdName.find ("All");
	if (pos == 0)
		return;
	_line += ' ';
	_line += files->GetFileList ();
}
