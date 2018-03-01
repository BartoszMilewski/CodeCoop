//------------------------------------
//  (c) Reliable Software, 2003 - 2007
//------------------------------------

#include "precompiled.h"
#include "ProjectMarker.h"
#include "Catalog.h"
#include "PathRegistry.h"

#include <File/File.h>
#include <File/Path.h>
#include <File/MemFile.h>

#include <iostream>



ProjectMarker::ProjectMarker (Catalog const & catalog, int projectId, std::string const & markerName)
{
	// Per project marker
	Assert (projectId > 0);
	FilePath database (catalog.GetProjectDataPath (projectId));
	_markerPath.assign (database.GetFilePath (markerName));
}

GlobalMarker::GlobalMarker (std::string const & markerName)
{
	// All projects database marker
	FilePath database (Registry::GetCatalogPath ());
	_markerPath.assign (database.GetFilePath (markerName));
}

char const * RepairList::_fileName = "RepairList.txt";
char const * RepairList::_delimiter = " ";

RepairList::RepairList ()
{
	InitializeFilePath ();

	if (Exists ())
	{
		std::ifstream inFile (_filePath.c_str ());
		std::istream_iterator<int> in (inFile);
		std::istream_iterator<int> inEnd;
		std::copy (in, inEnd, std::back_inserter (_projectIds));
	}
}

RepairList::RepairList (Catalog & catalog)
{
	InitializeFilePath ();

	for (ProjectSeq seq (catalog); !seq.AtEnd (); seq.Advance ())
	{
		if (seq.IsProjectUnavailable ())
			continue;

		_projectIds.push_back (seq.GetProjectId ());
	}
}

bool RepairList::Exists () const
{
	return File::Exists (_filePath);
}

std::vector<int> RepairList::GetProjectIds () const
{
	return _projectIds;
}

void RepairList::Save ()
{
	SaveTo (_filePath.c_str ());
}

void RepairList::Remove (int projectId)
{
	std::vector<int>::iterator iter = std::find (_projectIds.begin (), _projectIds.end (), projectId);
	Assert (iter != _projectIds.end ());
	_projectIds.erase (iter);
	if (_projectIds.empty ())
		File::DeleteNoEx (_filePath);
	else
		SaveTo (_filePath.c_str ());
}

void RepairList::Delete ()
{
	File::DeleteNoEx (_filePath);
}

void RepairList::InitializeFilePath ()
{
	FilePath globalDbPath (Registry::GetCatalogPath ());
	_filePath = globalDbPath.GetFilePath (_fileName);
}

void RepairList::SaveTo (char const * path)
{
	OutStream outFile (path);
	std::ostream_iterator<int> out (outFile, _delimiter);
	std::copy (_projectIds.begin (), _projectIds.end (), out);
}

void FileMarker::SetMarker (bool create)
{
	if (create)
	{
		MemFileAlways marker (_markerPath);
	}
	else
	{
		File::DeleteNoEx (_markerPath.c_str ());
	}
}

bool FileMarker::Exists () const
{
	return File::Exists (_markerPath.c_str ());
}

