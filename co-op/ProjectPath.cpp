//------------------------------------
//	(c) Reliable Software, 1998 - 2006
//------------------------------------

#include "precompiled.h"
#include "ProjectPath.h"
#include "DataBase.h"

Project::Path::Path (FileIndex const & fileIndex)
	: _fileIndex (fileIndex)
{}

Project::Path::Path (Path const & path)
	: FilePath (path.GetDir ()),
	  _fileIndex (path._fileIndex)
{}

char const * Project::Path::MakePath (GlobalId gid)
{
	FileData const * file = _fileIndex.FindByGid (gid);
	if (file == 0)
	{
		GlobalIdPack gidPack (gid);
		ResetLen ();
		Append (gidPack.ToBracketedString ());
	}
	else if (!file->GetType ().IsRoot ())
	{
		return MakePath (file->GetUniqueName ());
	}
	else
		ResetLen ();

	return c_str ();
}

char const * Project::Path::MakePath (UniqueName const & uname)
{
	std::vector<std::string> pathElements;
	pathElements.push_back (uname.GetName ());
	if (uname.GetParentId () != gidInvalid)
	{
		FileData const * folder = _fileIndex.FindByGid (uname.GetParentId ());
		if (folder != 0)
		{
			while (folder->GetType ().IsFolder ())
			{
				UniqueName const & folderUname = folder->GetUniqueName ();
				pathElements.push_back (folderUname.GetName ());
				folder = _fileIndex.FindByGid (folderUname.GetParentId ());
				if (folder == 0)
				{
					GlobalIdPack gid (folderUname.GetParentId ());
					pathElements.push_back (gid.ToBracketedString ());
					break;
				}
			}
		}
		else
		{
			GlobalIdPack gid (uname.GetParentId ());
			pathElements.push_back (gid.ToBracketedString ());
		}
	}

	int count = pathElements.size ();
	ResetLen ();
	for (int i = 0; i < count; i++)
	{
		Append (pathElements.back ());
		pathElements.pop_back ();
	}
	return c_str ();
}

Project::XPath::XPath (DataBase const & dataBase)
	: _dataBase (dataBase)
{}

Project::XPath::XPath (XPath const & path)
	: FilePath (path.GetDir ()),
	  _dataBase (path._dataBase)
{}

char const * Project::XPath::XMakePath (GlobalId gid)
{
	FileData const * file = _dataBase.XGetFileDataByGid (gid);
	return XMakePath (file->GetUniqueName ());
}

char const * Project::XPath::XMakePath (UniqueName const & uname)
{
	std::vector<std::string> pathElements;

	pathElements.push_back (uname.GetName ());
	FileData const * folder = _dataBase.XGetFileDataByGid (uname.GetParentId ());
	Assert (folder != 0);
	while (folder->GetType ().IsFolder ())
	{
		UniqueName const & folderUname = folder->GetUniqueName ();
		pathElements.push_back (folderUname.GetName ());
		folder = _dataBase.XGetFileDataByGid (folderUname.GetParentId ());
		Assert (folder != 0);
	}

	int count = pathElements.size ();
	ResetLen ();
	for (int i = 0; i < count; i++)
	{
		Append (pathElements.back ());
		pathElements.pop_back ();
	}
	return c_str ();
}
