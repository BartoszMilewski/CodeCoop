#if !defined (FILEDISPLAYTABLE_H)
#define FILEDISPLAYTABLE_H
//------------------------------------------------
// (c) Reliable Software 2002 -- 2004
// -----------------------------------------------

#include "GlobalId.h"
#include "FileTypes.h"

class FileDisplayTable
{
public:
	virtual ~FileDisplayTable () {}

	virtual GidSet const & GetFileSet () const = 0;
	virtual void Open (GidList const & gids) = 0;
	virtual void OpenAll () {};
	virtual void Cleanup (GidList const & gid) {};
	virtual char const * GetRootRelativePath (GlobalId gid) const = 0;
	virtual char const * GetFileName (GlobalId gid) const = 0;
	virtual FileType GetFileType (GlobalId gid) const = 0;
	virtual bool IsFolder (GlobalId gid) const = 0;

public:
	enum ChangeState
	{
		Unchanged,
		Changed,
		Renamed,
		Moved,
		Deleted,
		Created,
		NotPresent,
		Unrecoverable
	};

	virtual ChangeState GetFileState (GlobalId gid) const = 0;

	static std::string GetStateName (ChangeState state);
};

#endif
