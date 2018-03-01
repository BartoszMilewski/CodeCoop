//-----------------------------------
// (c) Reliable Software 2004 -- 2005
//-----------------------------------
#include "precompiled.h"
#include "CoopMemento.h"
#include "ProjectData.h"
#include "Catalog.h"
#include "AppInfo.h"

#include <Com/Shell.h>
#include <File/File.h>
#include <File/Dir.h>

NewProjTransaction::NewProjTransaction (Catalog & catalog, Project::Data const & projData)
	: _catalog (catalog), 
	_projData (projData),
	_rootDirExisted (true),
	_commit (false)
{
	_rootDirExisted = File::Exists (projData.GetRootDir ());
	if (_rootDirExisted)
	{
		// if target directory empty, do full cleanup upon abort
		FileSeq seq;
		if (seq.AtEnd ())
			_rootDirExisted = true;
	}
}

NewProjTransaction::~NewProjTransaction ()
{
	if (!_commit)
	{
		try
		{
			_catalog.UndoNewProject (_projData.GetProjectId ());
			char const * newRoot = _projData.GetRootDir ();
			if (File::Exists (newRoot))
			{
				if (!_rootDirExisted)
				{
					// Full tree delete
					ShellMan::Delete (TheAppInfo.GetWindow (), newRoot);
				}
				else
				{
					_abortRequest.Cleanup ();	// Delete only copied files
					std::vector<std::string>::iterator it;
					for (it = _createdFolders.begin (); it != _createdFolders.end (); ++it)
					{
						if (File::CleanupTree (it->c_str ()))
						{
							// Folder is empty -- remove it!
							::RemoveDirectory (it->c_str ());
						}
					}
				}
			}
		}
		catch (...) {}
	}
}