#if !defined (DBHANDLER_H)
#define DBHANDLER_H
//-----------------------------------------
//  (c) Reliable Software, 2005
//-----------------------------------------

#include <File/Path.h>

class CoopDbHandler
{
public:
	CoopDbHandler (FilePath const & root)
		: _root (root)
	{}

	bool CreateDb ();
	void DeleteDb ();

private:
	static char const *	_folderNames [];
	FilePath			_root;
};

#endif
