#if !defined (DIR_H)
#define DIR_H
//-------------------------------------
// (c) Reliable Software 1998 -- 2002
//-------------------------------------

#include <File/File.h>
#include <File/Path.h>
#include <File/Vpath.h>

class FileSeq
{
public:
	explicit FileSeq (char const * patternPath = 0);
	explicit FileSeq (std::string const & patternPath);
    virtual ~FileSeq () { Close (); }
    bool AtEnd () const;
	virtual void Advance ();
    char const * GetName () const;
	FileTime GetWriteTime () const { return _data.ftLastWriteTime; }
	unsigned long GetSize () const { return _data.nFileSizeLow; }
	bool IsFolder () const { return (_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
	bool IsReadOnly () const { return (_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0; }
	bool IsDots () const 
	{ 	
		return _data.cFileName [0] == '.' && (_data.cFileName [1] == '\0' // single dot
				|| (_data.cFileName [1] == '.' && _data.cFileName [2] == '\0')); // double dot
	}
protected:
	void Open (char const * pattern);
	void Close ();
protected:
    bool            _atEnd;
	::HANDLE          _handle;
	::WIN32_FIND_DATA _data;
};

class FileMultiSeq: public FileSeq
{
public:
	typedef std::vector<std::string> Patterns;
public:
	FileMultiSeq (FilePath const & dir, FileMultiSeq::Patterns const & patterns);
	FileMultiSeq (File::Vpath const & dir, FileMultiSeq::Patterns const & patterns);
	void Advance ();
	// AtEnd () inerited
private:
	void Init ();
	void NextPattern ();
private:
	FileMultiSeq::Patterns::const_iterator _curPattern;
	FileMultiSeq::Patterns::const_iterator _endPattern;
	File::Vpath _dir;
};

class DirSeq: public FileSeq
{
public:
    DirSeq (char const *pattern);
	void Advance ();
};

#endif
