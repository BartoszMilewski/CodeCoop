#if !defined (PROJECTPATH_H)
#define PROJECTPATH_H
//------------------------------------
//  (c) Reliable Software, 1998 - 2006
//------------------------------------

#include "GlobalId.h"

#include <File/Path.h>

class FileIndex;
class DataBase;
class UniqueName;

namespace Project
{
	class Path : public FilePath
	{
	public:
		Path (FileIndex const & fileIndex);
		Path (Project::Path const & path);

		char const * MakePath (GlobalId gid);
		char const * MakePath (UniqueName const & uname);

	private:
		FileIndex const & _fileIndex;
	};

	class XPath : public FilePath
	{
	public:
		XPath (::DataBase const & dataBase);
		XPath (Project::XPath const & path);

		char const * XMakePath (GlobalId gid);
		char const * XMakePath (UniqueName const & uname);

	private:
		DataBase const & _dataBase;
	};
}

#endif
