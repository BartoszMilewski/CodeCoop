#if !defined (TESTFILE_H)
#define TESTFILE_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include <File/File.h>
#include <File/Path.h>

class LocalTestFile
{
public:
	LocalTestFile ()
		: _created (true)
	{
		TmpPath tmpPath;
		_path = tmpPath.GetFilePath ("LocalTestFile.txt");
		CreateIfNecessary ();
	}
	LocalTestFile (std::string const & path)
		: _created (false),
		  _path (path)
	{
		CreateIfNecessary ();
	}
	~LocalTestFile ()
	{
		if (_created)
			File::DeleteNoEx (_path);
	}

	std::string const & GetPath () const { return _path; }

private:
	void CreateIfNecessary ()
	{
		if (File::Exists (_path))
		{
			File file (_path, File::OpenExistingMode ());
		}
		else
		{
			File file (_path, File::CreateAlwaysMode ());
			_created = true;
		}
	}

private:
	bool		_created;
	std::string	_path;
};

#endif
