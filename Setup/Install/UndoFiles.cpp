//-----------------------------------------
// (c) Reliable Software 1997, 98, 99, 2000
//-----------------------------------------

#include "precompiled.h"
#include "UndoFiles.h"

#include <File/File.h>
#include <Com/Shell.h>
#include <Dbg/Assert.h>

UndoTransferFiles::~UndoTransferFiles()
{
	if (!_commit)
		Delete (_created, _createdIsFolder);
}

void UndoTransferFiles::RememberCreated (char const * fullPath, bool isFolder)
{
	Assert (!_commit);
	_created.push_back (fullPath);
	_createdIsFolder.push_back (isFolder);
}

void UndoTransferFiles::Commit ()
{
	_commit = true;
}

void UndoTransferFiles::Delete (std::vector<std::string> & fullPath, std::vector<bool> & isFolder)
{
	Assert (fullPath.size () == isFolder.size ());
	for (unsigned int i = 0; i < fullPath.size (); i++)
	{
		if (isFolder [i])
		{
			// We do not pass here the parent hwnd to the Shell
			// for quiet operation
			ShellMan::QuietDelete (0, fullPath [i].c_str ());
		}
		else
		{
			File::DeleteNoEx (fullPath [i].c_str ());
		}
	}
}

