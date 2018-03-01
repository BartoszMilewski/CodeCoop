#if !defined (DRIVES_H)
#define DRIVES_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2007
// ----------------------------------

#include <StringOp.h>

class DriveSeq
{
public:
	DriveSeq ();
	bool AtEnd () { return _cur == _end; }
	virtual void Advance () { ++_cur; }
	char const * GetDriveString () const { return *_cur; }

protected:
	MultiString					_driveStrings;
	MultiString::const_iterator	_cur;
	MultiString::const_iterator	_end;
};

class WriteableDriveSeq : public DriveSeq
{
public:
	void Advance ();
};

class DriveInfo
{
public:
	DriveInfo (std::string const & rootPath);
	bool IsUnknown () const { return _type == DRIVE_UNKNOWN; }
	bool HasNoRoot () const { return _type == DRIVE_NO_ROOT_DIR; }
	bool IsRemovable () const { return _type == DRIVE_REMOVABLE; }
	bool IsFixed () const { return _type == DRIVE_FIXED; }
	bool IsRemote () const { return _type == DRIVE_REMOTE; }
	bool IsCdRom () const { return _type == DRIVE_CDROM; }
	bool IsRamDisk () const { return _type == DRIVE_RAMDISK; }

	int MbytesFree () const;
	
private:
	std::string	_rootPath;
	UINT		_type;
};

#endif
