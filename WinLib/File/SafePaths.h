#if !defined (SAFEPATHS_H)
#define SAFEPATHS_H
// ----------------------------------
// (c) Reliable Software, 1999 - 2007
// ----------------------------------

#include <File/Path.h>

class SafePaths
{
public:
	~SafePaths ();
	void MakeTmpCopy (char const * src, char const * dst);
	bool MakeTmpCopyNoEx (char const * src, char const * dst);
	void Remember (char const * path);
	void Remember (std::string const & path);

	bool IsEmpty () const { return _paths.empty (); }
	unsigned size () const { return _paths.size (); }
	std::string const & back () const { return _paths.back (); }

	typedef std::vector<std::string>::const_iterator iterator;
	iterator begin () const { return _paths.begin (); }
	iterator end () const { return _paths.end (); }
private:

	std::vector<std::string> _paths;
};

class SafeTmpFile
{
public:
	SafeTmpFile (std::string const & fileName)
		: _fileName (fileName),
		  _commit (false)
	{}
	SafeTmpFile ()
		: _commit (false)
	{}

	~SafeTmpFile ();

	void SetFileName (std::string const & name) { _fileName = name; }
	void SetFilePath (std::string const & path) { _path.Change (path); }

	void Commit () { _commit = true; }
	char const * GetDirPath () const
	{
		return _path.GetDir ();
	}
	char const * GetDrive () const
	{
		return _path.GetDrive ().c_str();
	}
	char const * GetFilePath () const
	{
		return _path.GetFilePath (_fileName);
	}
private:
	TmpPath		_path;
	std::string _fileName;
	bool		_commit;
};

#endif
