#if !defined (FILELOCKER_H)
#define FILELOCKER_H
//---------------------------
// (c) Reliable Software 2010
//---------------------------

#include <File/Path.h>
#include <Sys\Synchro.h>

class ScriptTicket;

// Serializes operations on scripts in the PublicInbox
// Makes temporary copies and caches them
class FileLocker
{
	struct TmpCopy
	{
		TmpCopy() {}
		TmpCopy(std::string const & path) : _path(path), _refCount(1)
		{}
		std::string _path;
		int _refCount;
	};
public:
	struct Guard
	{
		Guard(FileLocker & locker, std::string const & fileName)
			: _locker(locker), _fileName(fileName)
		{}
		~Guard()
		{
			_locker.ReleaseFileCopy(_fileName);
		}
		FileLocker & _locker;
		std::string _fileName;
	};
public:
	void StampFinalDelivery(ScriptTicket & script, int idx);
	std::string AcquireFileCopy(std::string const & srcPath, std::string const & fileName, bool isFwd, bool goesToHub);
	void ReleaseFileCopy(std::string const & fileName);
private:
	Win::CritSection	_critSection;
	TmpPath				_tmpFolder;
	std::map<std::string, TmpCopy> _tmpCopies;
};

extern FileLocker TheFileLocker;

#endif
