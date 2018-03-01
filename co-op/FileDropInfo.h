#if !defined (FILEDROPINFO_H)
#define FILEDROPINFO_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

#include "FileCtrlState.h"
#include "GlobalId.h"

class ControlledFileDropInfo : public DropInfo
{
public:
	ControlledFileDropInfo (std::string const & sourcePath,
							std::string const & targetPath,
							GlobalId gid,
							FileControlState state)
		: DropInfo (sourcePath, targetPath, state),
		  _gid (gid)
	{}

	bool IsControlled () const { return true; }

	bool RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const ;

private:
	GlobalId	_gid;
};

class ControlledFolderDropInfo : public DropInfo
{
public:
	ControlledFolderDropInfo (std::string const & sourcePath,
							  std::string const & targetPath,
							  GlobalId gid,
							  FileControlState state)
		: DropInfo (sourcePath, targetPath, state),
		  _gid (gid)
	{}

	bool IsControlled () const { return true; }
	bool IsFolder () const { return true; }

	bool RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const;

private:
	GlobalId	_gid;
};

class NotControlledFileDropInfo : public DropInfo
{
public:
	NotControlledFileDropInfo (std::string const & sourcePath,
							   std::string const & targetPath,
							   FileControlState state)
		: DropInfo (sourcePath, targetPath, state)
	{}

	bool RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const;
};

class NotControlledFolderDropInfo : public DropInfo
{
public:
	NotControlledFolderDropInfo (std::string const & sourcePath,
								 std::string const & targetPath,
								 FileControlState state)
		: DropInfo (sourcePath, targetPath, state)
	{}

	bool IsFolder () const { return true; }

	bool RecordDropInfo (FileDropInfoCollector & collector, DropPreferences & userPrefs) const;
};

#endif
