#if !defined (BACKUPFILE_H)
#define BACKUPFILE_H
//-----------------------------------
// (c) Reliable Software, 1998 - 2007
//-----------------------------------

#include <File/Path.h>

class BackupFile
{
public:
	BackupFile (std::string const & originalPath);
	~BackupFile ();
	void Commit ();

private:
	FilePath	_path;
	std::string	_original;
	std::string	_backup;
	bool		_commit;
};

#endif
