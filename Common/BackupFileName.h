#if !defined (BACKUPFILENAME_H)
#define BACKUPFILENAME_H
//----------------------------------
//  (c) Reliable Software, 2008
//----------------------------------

class BackupFileName
{
public:
	BackupFileName ();

	std::string const & Get () const { return _fileName; }

private:
	std::string	_fileName;
};

#endif
