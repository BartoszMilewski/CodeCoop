//------------------------------------
//  (c) Reliable Software, 2002 - 2007
//------------------------------------

#include "precompiled.h"
#include "ExtensionScanner.h"
#include "DirTraversal.h"

#include <Ctrl/ProgressMeter.h>
#include <File/File.h>
#include <File/Dir.h>

ExtensionScanner::ExtensionScanner (std::string const & root,
									Progress::Meter & progressMeter)
	: _root (root)
{
	int fileCount = 0;
	int folderCount = 0;
	// Count files and folders at root level
	for (::FileSeq seq (_root.GetAllFilesPath ()); !seq.AtEnd (); seq.Advance ())
	{
		if (File::IsFolder (_root.GetFilePath (seq.GetName ())))
			folderCount++;
		else
			fileCount++;
	}
	if (folderCount + fileCount > 0)
	{
		progressMeter.SetRange (0, folderCount, 1);
		Traversal collecting (_root, *this, progressMeter);
	}
}

bool ExtensionScanner::Visit (char const * path)
{
	bool isFolder = File::IsFolder (path);
	if (!isFolder)
	{
		// Collect info about file
		PathSplitter splitter (path);
		if (splitter.HasExtension ())
		{
			_extensions.insert (splitter.GetExtension ());
		}
	}
	return isFolder;
}
