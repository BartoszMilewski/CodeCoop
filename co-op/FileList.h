#if !defined FILELIST_H
#define FILELIST_H
//------------------------------------
// (c) Reliable Software 1997 -- 2002
//------------------------------------

#include "GlobalId.h"

namespace Win { class ExitException; }

class TransactionFileList
{
public:
	TransactionFileList ()
		: _commit (false)
	{}
	~TransactionFileList ();

	void RememberDeleted (char const * fullPath, bool isFolder);
	void RememberCreated (char const * fullPath, bool isFolder);
	void RememberToMove (GlobalId gid, char const * fromPath, char const * toPath, bool touch);
	void RememberToMakeReadOnly (GlobalId gid, char const * path);
	void RememberToMakeReadWrite (char const * path) 
	{ 
		_makeReadWrite.push_back (path);
	}
	void CommitPhaseOne ();
	void CommitOrDie (); //  throw (Win::ExitException);
	void SetOverwrite (GlobalId gid, bool flag);

private:
	struct PathInfo
	{
		PathInfo (char const * path, bool isFolder)
			: _path (path),
			  _isFolder (isFolder)
		{}

		std::string	_path;
		bool		_isFolder;
	};

	struct MoveInfo
	{
		MoveInfo (GlobalId gid, bool overwrite, bool touch, char const * from, char const * to)
			: _gid (gid),
			  _overwrite (overwrite),
			  _touch (touch),
			  _readOnly (false),
			  _from (from),
			  _to (to)
		{}

		GlobalId	_gid;
		bool		_overwrite;
		bool		_touch;
		bool		_readOnly;
		std::string	_from;
		std::string	_to;
	};

	class IsEqualGid : public std::unary_function<MoveInfo const &, bool>
	{
	public:
		IsEqualGid (GlobalId gid)
			: _gid (gid)
		{}

		bool operator () (MoveInfo const & info) const
		{
			return info._gid == _gid;
		}
	private:
		GlobalId	_gid;
	};

	typedef std::vector<MoveInfo>::iterator MoveIterator;

private:
	static void DeletePath (PathInfo const & path);
	static void PrepareMove (MoveInfo const & path);
	static void Move (MoveInfo const & path); //  throw (Win::ExitException);

private:
	std::vector<PathInfo>		_deleted;
	std::vector<PathInfo>		_created;
	std::vector<MoveInfo>		_moved;
	std::vector<std::string>	_makeReadOnly;
	std::vector<std::string>	_makeReadWrite;

	bool						_commit;
};

#endif
