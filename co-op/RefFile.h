#if !defined reffile_h
#define reffile_h
//----------------------------------
// (c) Reliable Software 1997 - 2005
//----------------------------------

#include "GlobalId.h"
#include "CheckSum.h"

class Path;
class DiffCmd;
class FileData;

class ReferenceFile
{
public:
	ReferenceFile (char const * refPath);

	char const * GetPath () { return _refPath.c_str (); }
	// Warning: may be called only once
	std::unique_ptr<DiffCmd> GetCmd (){ return std::move(_recorder);}
	bool Diff (char const * newVersionPath, FileData const & fileData);
	CheckSum GetNewCheckSum () const { return _newCheckSum; }

private:
	std::string				_refPath;
	std::unique_ptr<DiffCmd>	_recorder;
	CheckSum				_newCheckSum;
};

#endif
