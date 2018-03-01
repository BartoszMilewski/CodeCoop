#if !defined (SYSPATH_H)
#define SYSPATH_H
//----------------------------------
// (c) Reliable Software 1997 - 2009
//----------------------------------

#include <File/MemFile.h>
#include <File/Path.h>

class SysPathFinder
{
public:
    SysPathFinder ()
        : _dataDir ()
    {}

	void Init (FilePath const & path) { _dataDir.Change (path); }
    void Clear () { _dataDir.Clear (); }

    char const * GetSysPath () const { return _dataDir.GetDir (); }
	char const * GetSwitchPath () const { return _dataDir.GetFilePath (SwitchFileName); }
    char const * GetSysFilePath (char const * fileName) const { return _dataDir.GetFilePath (fileName); }
    char const * GetAllSysFilesPath () const { return _dataDir.GetAllFilesPath (); }
    char const * GateName () const { return _dataDir.GetFilePath (LockFileName); }

    bool LockProject ();
	bool IsProjectLocked () throw ();
    void UnlockProject () throw ();
    void CreateLock ();
	bool IsLockStamped () const { return _lokFile.GetSize ().Low () == sizeof (long); }
	bool IsStampEqual (long timeStamp) const { return _lokFile.GetStamp () == timeStamp; }
	bool IsStampLess (long timeStamp) const { return _lokFile.GetStamp () < timeStamp; }
	void StampLockFile (long timeStamp) { _lokFile.Stamp (timeStamp); }
    void RemoveLock ();

	bool IsInProject () const { return !_dataDir.IsDirStrEmpty (); }

private:
    static char const LockFileName [];
    static char const SwitchFileName [];

    LokFile  _lokFile;
    FilePath _dataDir;
};

#endif
