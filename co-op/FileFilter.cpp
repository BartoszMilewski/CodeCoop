//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "precompiled.h"
#include "FileFilter.h"
#include "SelectIter.h"
#include "Directory.h"
#include "FileTag.h"

FileFilter::FileFilter (SelectionSeq & seq, Directory & folder)
{
	if (seq.Count () == 1 && seq.GetType ().IsFolder ())
	{
		// File filter constructed from one selected folder
		// Make the folder path a filter pattern
		GlobalId gid = seq.GetGlobalId ();
		if (gid != gidInvalid)
		{
			std::string projectPath (folder.GetRootRelativePath (gid));
			_filter [gid] = projectPath.c_str ();
			SetFilterPattern (projectPath);
			return;
		}
	}
	_filterPattern.clear ();
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		GlobalId gid = seq.GetGlobalId ();
		if (gid != gidInvalid)
		{
			std::string projectPath (folder.GetRootRelativePath (gid));
			_filter [gid] = projectPath.c_str ();
			_filterPattern += '"';
			_filterPattern += projectPath;
			_filterPattern += "\" ";
		}
	}
}

FileFilter::FileFilter (FileFilter const & filter)
{
	_filter.insert (filter._filter.begin (), filter._filter.end ());
	_filterPattern = filter._filterPattern;
	_scripts.insert (filter._scripts.begin (), filter._scripts.end ());
}

void FileFilter::AddFile (GlobalId gid, std::string const & projectPath)
{
	_filter [gid] = projectPath;
}

void FileFilter::AddFiles (auto_vector<FileTag> const & files)
{
	for (auto_vector<FileTag>::const_iterator iter = files.begin ();
		 iter != files.end ();
		 ++iter)
	{
		_filter [(*iter)->Gid ()] = (*iter)->Path ();
	}
}

bool FileFilter::IsFilteringOn () const
{
	return IsFileFilterOn () || IsScriptFilterOn ();
}

bool FileFilter::IsIncluded (GlobalId gid) const
{
	FilterIter iter = _filter.find (gid);
	return iter != _filter.end ();
}

bool FileFilter::IsScriptIncluded (GlobalId gid) const
{
	GidSet::const_iterator iter = _scripts.find (gid);
	return iter != _scripts.end ();
}

bool FileFilter::IsEqual (FileFilter const & ff) const
{
	return _filter == ff._filter &&
			_scripts == ff._scripts &&
			_filterPattern == ff._filterPattern;
}
