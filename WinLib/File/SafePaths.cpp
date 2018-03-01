// --------------------------------
// (c) Reliable Software, 1999-2007
// --------------------------------
#include <WinLibBase.h>
#include "SafePaths.h"
#include <File/File.h>

SafePaths::~SafePaths ()
{
	for (iterator it = begin (); it != end (); ++it)
		File::DeleteNoEx (it->c_str ());
}

void SafePaths::Remember (char const * path)
{
	_paths.push_back (std::string (path));
}

void SafePaths::Remember (std::string const & path)
{
	_paths.push_back (path);
}

void SafePaths::MakeTmpCopy (char const * src, char const * dst)
{
	_paths.push_back (dst);
	File::Copy (src, dst);
}

bool SafePaths::MakeTmpCopyNoEx (char const * src, char const * dst)
{
	if (!File::Exists (src))
		return false;

	_paths.push_back (dst);
	return File::CopyNoEx (src, dst);
}

SafeTmpFile::~SafeTmpFile ()
{
	if (!_commit && _fileName.size() != 0)
		File::DeleteNoEx (GetFilePath ());
}

