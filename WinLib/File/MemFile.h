#if !defined MEMFILE_H
#define MEMFILE_H
//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "File.h"

class MappableFile: public File
{
public:
	MappableFile (std::string const & path, File::Mode mode);
	MappableFile (std::string const & path, File::Mode mode, File::Attributes attributes);
	MappableFile (std::string const & path, File::Mode mode, File::Attributes attributes, File::Size size);
    MappableFile (File & file, bool readOnly);
	~MappableFile () throw () { Close (); }
	void Close () throw ();
	bool IsFile () const { return FileOk (); }
	HANDLE ToNative () const { return _hMap; }
	void MakeMap (File::Size size, bool readOnly);
	void MakeMap (bool readOnly);
	void CloseMap () throw ();
protected:
	MappableFile () :_hMap (0) {}
protected:
	HANDLE			_hMap;
};

class FileViewRo : public MappableFile
{
public:
	FileViewRo (std::string const & path);
	~FileViewRo();

	char const *  GetBuf (File::Offset off, unsigned len);

protected:
	unsigned		_pageSize;
	char const		* _viewBuf;
};

class FileViewRoSeq : public FileViewRo
{
public:
	FileViewRoSeq (std::string const & path);

	bool AtEnd () const { return _cur == _end; }
	void Advance ();

    char const *  GetBuf () const { return _viewBuf; }
	unsigned long GetBufSize () const { return _viewSize; }

private:
	LargeInteger	_end;
	LargeInteger	_cur;
	unsigned long	_viewSize;
};

class MemFile: public MappableFile
{
public:
	enum { MAX_BUFFER = 0x40000000 }; // 1 GB

public:
	MemFile (std::string const & path, File::Mode mode, File::Size size);
	MemFile (std::string const & path, File::Mode mode);
    MemFile (File & file, bool readOnly);
    ~MemFile() throw ();
	void Close () throw ();
	void Close (File::Size finalSize);
    char *  GetBuf () { return _buf; }
    char const *  GetBuf () const { return _buf; }
	unsigned long GetBufSize () const
		{ return _bufSize; }
	void Flush ();
	// invalidates previous buffer
	char * ResizeFile (File::Size size);
	char * Reallocate ( unsigned long bufSize, File::Offset offset);
    unsigned long GetCheckSum () const;

protected:
	MemFile () : _readOnly (true), _buf (0), _bufSize (0) {}
	void InitBufSize (File::Size size);
	void InitBufSize ();
	void Allocate ( File::Offset fileOffset = File::Offset (0, 0));
	void Deallocate () throw ();

protected:
	bool			_readOnly;
	char		  * _buf;
	unsigned long	_bufSize;
};

class MemFileExclusiveReadOnly : public MemFile
{
public:
    MemFileExclusiveReadOnly (std::string const & path)
		: MemFile (path, File::ExclusiveReadOnlyMode ())
	{}
};

class MemFileReadOnly: public MemFile
{
public:
    MemFileReadOnly (std::string const & path)
		: MemFile (path, File::ReadOnlyMode ())
	{}
    MemFileReadOnly (File & file);
};

class MemFileNew: public MemFile
{
public:
	MemFileNew (std::string const & path, File::Size size)
		: MemFile (path, File::CreateAlwaysMode (), size)
	{}
};

class MemFileExisting: public MemFile
{
public:
	MemFileExisting (std::string const & path, File::Mode mode = File::OpenExistingMode())
		: MemFile (path, mode)
	{}
};

class MemFileAlways: public MemFile
{
public:
	MemFileAlways (std::string const & path)
		: MemFile (path, File::OpenAlwaysMode ())
	{}
};

class LokFile: public MemFile
{
public:
    LokFile () {}
    void New (char const *path);
    bool Open (char const *path);
	void Stamp (long timeStamp);
	long GetStamp () const { return *(reinterpret_cast<long const *> (_buf)); }

	static bool IsLocked (char const *path) throw ();
};

#endif
