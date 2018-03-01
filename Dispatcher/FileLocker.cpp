//---------------------------
// (c) Reliable Software 2010
//---------------------------

#include "precompiled.h"
#include "FileLocker.h"
#include "ScriptInfo.h"
#include "TransportManager.h"

FileLocker TheFileLocker;

void FileLocker::StampFinalDelivery(ScriptTicket & script, int idx)
{
	Win::Lock lock(_critSection);
	script.StampFinalDelivery (idx); // sets the _stampedRecipients entry
}

std::string FileLocker::AcquireFileCopy(std::string const & srcPath, std::string const & fileName, bool isFwd, bool goesToHub)
{
	Win::Lock lock(_critSection);
	std::map<std::string, TmpCopy>::iterator it = _tmpCopies.find(fileName);
	if (it != _tmpCopies.end())
	{
		++it->second._refCount;
		return it->second._path;
	}

	std::string tmpPath = _tmpFolder.GetFilePath (fileName);
	if (!File::CopyNoEx (srcPath.c_str (), tmpPath.c_str ()))
		return std::string();

	if (isFwd != goesToHub)
		TransportManager::StampFwdFlag (tmpPath, goesToHub);


	_tmpCopies[fileName] = TmpCopy(tmpPath);
	return tmpPath;
}

void FileLocker::ReleaseFileCopy(std::string const & fileName)
{
	Win::Lock lock(_critSection);
	std::map<std::string, TmpCopy>::iterator it = _tmpCopies.find(fileName);
	Assert (it != _tmpCopies.end());
	if (--it->second._refCount == 0)
	{
		File::DeleteNoEx(it->second._path);
		_tmpCopies.erase(it);
	}
}
