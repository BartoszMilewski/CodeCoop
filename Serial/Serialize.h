#if !defined SERIALIZE_H
#define SERIALIZE_H
//------------------------------------
//	(c) Reliable Software, 1996 - 2008
//------------------------------------

#include <File/FileIo.h>
#include <File/Path.h>
#include <auto_vector.h>

#include <climits>

const long inFileTrue  = 0xbaca;
const long inFileFalse = 0xdec;

namespace Win { class Exception; }

class Deserializer
{
public:
	virtual ~Deserializer () {}

	virtual unsigned char GetByte () = 0;
	virtual long GetLong () = 0;
	virtual bool GetBool () = 0;
	virtual void GetBytes (char * pBuf, unsigned long size) = 0;
	virtual void GetBytes (unsigned char * pBuf, unsigned long size) = 0;
	virtual File::Offset GetPosition () const = 0;
	virtual File::Offset SetPosition (File::Offset pos) = 0;
	virtual File::Size GetSize () = 0;
	virtual bool Rewind () = 0;
	virtual std::string const & GetPath () const { return _emptyPath; }

private:
	static std::string	_emptyPath;
};

class FileDeserializer : public Deserializer, public FileIo
{
	struct FileDeserializerRef
	{
		FileDeserializerRef (FileDeserializer const & des)
			: ref (des)
		{}
		File::File_Ref ref;
	};
	friend struct FileDeserializerRef;

public:
	FileDeserializer (std::string const & path, bool quiet = false)
		: _path (path)
	{
		OpenReadOnly (_path, quiet);
	}
	FileDeserializer (FileDeserializerRef dref)
		: FileIo (dref.ref)
	{}
	FileDeserializer ()
		: FileIo ()
	{}

	unsigned char GetByte ();
	long GetLong ();
	bool GetBool ();
	void GetBytes(char * pBuf, unsigned long size) { Read (pBuf, size); }
	void GetBytes(unsigned char * pBuf, unsigned long size) { Read (pBuf, size); }
	File::Offset GetPosition () const { return FileIo::GetPosition (); }
	File::Offset SetPosition (File::Offset pos) 
	{ 
		return FileIo::SetPosition (pos); 
	}
	File::Size GetSize () { return File::GetSize (); }
	bool Rewind () { return File::Rewind (); }
	std::string const & GetPath () const { return _path; }

private:
	std::string	_path;
};

class Serializer
{
public:
	virtual ~Serializer () {}

	virtual void PutByte (unsigned char b) = 0;
	virtual void PutLong (long l) = 0;
	virtual void PutBool (bool f) = 0;
	virtual void PutBytes(char const * pBuf, unsigned long size) = 0;
	virtual void PutBytes(unsigned char const * pBuf, unsigned long size) = 0;
	virtual bool IsCounting () const { return false; }

	static int SizeOfLong () 
	{
//Revisit: maybe there is better way of generating compile time error
//		   if sizeof(unsigned long) != 4 but I don't know it. You can't use
//		   sizeof (unsigned long) in the #if statement
#if ULONG_MAX != 0xffffffffUL
#error Size of long NOT EQUAL 4 -- serialization & deserialization will not work
#endif
		return 4;
	}
	static int SizeOfByte () 
	{ 
#if CHAR_BIT != 8
#error Size of byte NOT EQUAL 1 -- serialization & deserialization will not work
#endif
		return 1;
	}
	static int SizeOfBool () 
	{ 
		return Serializer::SizeOfLong ();
	}
};

class FileSerializer : public Serializer, public FileIo
{
public:
	FileSerializer (std::string const & path, File::Mode mode = File::OpenAlwaysMode ())
		: FileIo (path, mode)
	{}
	FileSerializer () : FileIo () {}
	
	void Create (std::string const & path)
	{
		File::Create (path);
	}

	void PutByte (BYTE b) 
	{
		Write (&b, SizeOfByte ()); 
	}
	void PutLong (long l) 
	{
		Write (&l, SizeOfLong ()); 
	}
	void PutBool (bool f);
	void PutBytes(char const * pBuf, unsigned long size) { Write (pBuf, size); }
	void PutBytes(unsigned char const * pBuf, unsigned long size) { Write (pBuf, size); }
};

class CountingSerializer : public Serializer
{
public:
	CountingSerializer () : _size (0) {}

	void PutByte (BYTE b) 
	{ 
		_size += SizeOfByte (); 
	}
	void PutLong (long l) 
	{
		_size += SizeOfLong ();
	}
	void PutBool (bool f)
	{
		_size += SizeOfBool (); 
	}
	void PutBytes(char const * pBuf, unsigned long size) { _size += size; }
	void PutBytes(unsigned char const * pBuf, unsigned long size) { _size += size; }
	bool IsCounting () const { return true; }
	long long GetSize () const { return _size; }
	void ResetSize () { _size = 0; }

private:
	long long _size;
};

class MemoryBuf
{
public:
	MemoryBuf (unsigned char * begin, unsigned long size)
		: _begin (begin),
		  _end (_begin + size)
	{
		if (size == 0)
			_end = 0;	// Buffer size not specified -- disable boundry checking
	}

	bool IsFreeSpace (int numberOfBytes) const
	{
		if (_end == 0)
			return true;
		return _begin + numberOfBytes <= _end;
	}
	unsigned char * GetWritePos () { return _begin; }
	unsigned char const * GetReadPos () const { return _begin; }
	void Advance (int numberOfBytes) { _begin += numberOfBytes; }

protected:
	unsigned char *	_begin;
	unsigned char *	_end;
};

class MemorySerializer : public MemoryBuf, public Serializer
{
public:
	MemorySerializer (unsigned char * buf, int size)
		: MemoryBuf (buf, size)
	{}

	void PutByte (unsigned char b);
	void PutLong (long l);
	void PutBool (bool f);
	void PutBytes (char const * pBuf, unsigned long size);
	void PutBytes (unsigned char const * pBuf, unsigned long size);
};

class MemoryDeserializer : public MemoryBuf, public Deserializer
{
public:
	MemoryDeserializer (unsigned char const * buf, unsigned long size = 0)
		: MemoryBuf (const_cast<unsigned char *>(buf), size),
		  _size (size),
		  _bufStart (buf)
	{}

	unsigned char GetByte ();
	long GetLong ();
	bool GetBool ();
	void GetBytes (char * pBuf, unsigned long size);
	void GetBytes (unsigned char * pBuf, unsigned long size);
	File::Offset GetPosition () const;
	File::Offset SetPosition (File::Offset pos);
	File::Size GetSize () { return File::Size (_size, 0); }
	bool Rewind ();

private:
	unsigned long			_size;
	unsigned char const *	_bufStart;
};

class LogSerializer : public FileSerializer
{
public:
	LogSerializer (std::string const & path, File::Offset cmdStart)
		: FileSerializer (path, File::OpenAlwaysShareReadMode ())
	{
		SetPosition (cmdStart);
	}
};

class LogDeserializer : public FileDeserializer
{
public:
	LogDeserializer (std::string const & path, File::Offset cmdStart)
		: FileDeserializer (path)
	{
		SetPosition (cmdStart);
	}
};

class Serializable
{
public:
	virtual ~Serializable () {}
	virtual int  VersionNo () const;
	virtual int  SectionId () const { return 0; }
	virtual bool IsSection () const { return false; }

	void Save (Serializer& out) const;
	long long Read (Deserializer& in);

protected:
	virtual void Serialize (Serializer& out) const = 0;
	virtual void Deserialize (Deserializer& in, int version) = 0;
	virtual void Deserialize (std::string const & inputPath, File::Offset startOffset, int version) {};
	virtual bool ConversionSupported (int versionRead) const { return true; }
	virtual void CheckVersion (int versionRead, int versionExpected) const;

private:
	bool FindSection (Deserializer& in, unsigned long sectionID);
};

template<class T> class LogItem;

template<class T>
class Log
{
	friend class LogItem<T>;

public:
	Log ();
	void Init (std::string const & path);
	void CreateFile ();
	void Clear ();
	void Copy (std::string const & path) const;
	void Delete ();
	bool Exists () const;
	char const * GetPath () const;
	File::Offset Append (File::Offset start, T const & item);
	std::unique_ptr<T> Retrieve (File::Offset start, int version) const;
	// bulk operations
	File::Offset Append (File::Offset start, 
						auto_vector<T> const & entries, 
						std::vector<File::Offset> & offsets);
	void Retrieve (std::vector<File::Offset> const & offsets, 
						auto_vector<T> & entries, 
						int version);

protected:
	FilePath	_logPath;
};

template <class T>
Log<T>::Log ()
{
	Clear ();
}

template <class T>
void Log<T>::Init (std::string const & path)
{
	_logPath.Change (path);
}

template <class T>
void Log<T>::CreateFile ()
{
	FileSerializer out;
	out.Create (_logPath.GetDir ());
}

template <class T>
void Log<T>::Clear ()
{
	_logPath.Clear ();
}

template <class T>
void Log<T>::Copy (std::string const & destPath) const
{
	File::Copy (_logPath.GetDir (), destPath.c_str ());
}

template <class T>
void Log<T>::Delete ()
{
	File::DeleteNoEx (_logPath.GetDir ());
	Clear ();
}

template <class T>
bool Log<T>::Exists () const
{
	return File::Exists (_logPath.GetDir ());
}

template <class T>
char const * Log<T>::GetPath () const
{
	return _logPath.GetDir ();
}

template <class T>
File::Offset Log<T>::Append (File::Offset start, T const & item)
{
	LogSerializer out (_logPath.GetDir (), start);
	item.Save (out);
	out.Flush ();
	return out.GetPosition ();
}

template <class T>
std::unique_ptr<T> Log<T>::Retrieve (File::Offset start, int version) const
{
	LogDeserializer in (_logPath.GetDir (), start);
	return std::unique_ptr<T> (new T (in, version));
}

// returns end offset
template <class T>
File::Offset Log<T>::Append (File::Offset start, 
							  auto_vector<T> const & entries, // entries in
							  std::vector<File::Offset> & offsets) // offsets out
{
	LogSerializer out (_logPath.GetDir (), start);
	Assert (offsets.size () == 0);
	unsigned count = entries.size ();
	offsets.reserve (count);
	for (unsigned i = 0; i < count; ++i)
	{
		offsets.push_back (out.GetPosition ());
		entries [i]->Save (out);
	}
	out.Flush ();
	return out.GetPosition ();
}

template <class T>
void Log<T>::Retrieve (std::vector<File::Offset> const & offsets, auto_vector<T> & entries, int version)
{
	unsigned count = offsets.size ();
	LogDeserializer in (_logPath.GetDir (), File::Offset (0, 0));
	for (unsigned i = 0; i < count; ++i)
	{
		in.SetPosition (offsets [i]);
		entries.push_back (std::unique_ptr<T> (new T (in, version)));
	}
}

#endif
