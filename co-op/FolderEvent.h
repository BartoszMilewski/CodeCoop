#if !defined (FOLDEREVENT_H)
#define FOLDEREVENT_H
//----------------------------------
//  (c) Reliable Software, 1999-2005
//----------------------------------

#include <File/Path.h>

enum RequestKind
{
	CheckContents,
	DeleteFolder
};

class FolderRequest
{
public:
	FolderRequest (std::string const & path, RequestKind kind = CheckContents)
		: _path (path),
		  _kind (kind)
	{}

	bool IsCheckContents () const { return _kind == CheckContents; }
	bool IsDeleteFolder () const { return _kind == DeleteFolder; }
	bool IsEqual (FolderRequest const & request) const
	{
		return request._kind == _kind && request._path.IsEqualDir (_path);
	}
	FilePath const & GetPath () const { return _path; }

public:
	FilePath	_path;
	RequestKind	_kind;
};

#endif
