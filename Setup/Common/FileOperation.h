#if !defined (FILEOPERATION_H)
#define FILEOPERATION_H
//------------------------------------
//  FileOperation.h  
//  (c) Reliable Software, 1999
//------------------------------------

class FilePath;

namespace Setup
{
	void DeleteFiles (char const * const fileList [], FilePath const & from);
	void DeleteFilesAndFolder (char const * const * files, FilePath const & folderPath);
	bool IsEmptyFolder (FilePath const & folder);
	void CloseRunningApp ();
	void DeleteIntegratorFiles (FilePath const & fromPath);
}

#endif
