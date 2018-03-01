//-----------------------------
//  (c) Reliable Software, 2005
//-----------------------------
#include "precompiled.h"
#include "Watch.h"
#include <File/Dir.h>
#include <File/Path.h>
#include <time.h>

Watch::Watch (Win::Dow::Handle win)
{
	std::srand (static_cast<unsigned int>(time (0)));
	FilePath path ("c:\\");
	DirSeq seq (path.GetAllFilesPath ());
	while (!seq.AtEnd ())
	{
		char const * filePath = path.GetFilePath (seq.GetName ());
		File::Attributes attr (filePath);
		if (!attr.IsSystem ())
			_folders.push_back (filePath);
		seq.Advance ();
	}
	_watcher.reset (new MultiFolderWatcher (_folders, win));
	_watcher.SetWaitForDeath (); // infinity
}

void Watch::RemoveFolder ()
{
	int i = std::rand () % _folders.size ();
	_watcher->StopWatching (_folders [i]);
}

void Watch::AddFolder ()
{
	int i = std::rand () % _folders.size ();
	_watcher->AddFolder (_folders [i], false); // non-recursive
}
