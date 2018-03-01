#if !defined NAMEDBLOCK_H
#define NAMEDBLOCK_H
//-------------------------------------
// (c) Reliable Software 1999, 2000, 01
// ------------------------------------

#include <auto_vector.h>
#include <File/Path.h>
#include <File/FileIo.h>

class InNamedBlock
{
public:
	InNamedBlock (std::string const & name, char const * buf, File::Size size)
		: _name (name),
		  _buf (buf),
		  _size (size)
	{}

    std::string const & GetName () const { return _name; }
	char const * GetBuf () const { return _buf; }
	File::Size GetSize () const { return _size; }

private:
	 std::string	_name;
	 const char *	_buf;
	 File::Size		_size;
};

class MemFileReadOnly;

class OutNamedBlock
{
public:
	virtual ~OutNamedBlock () {}

	virtual void Init (std::string const & newName) { _name = newName; }
    std::string const & GetName () const { return _name; }

	virtual void Write (unsigned char const * buf, unsigned int size) = 0;
	virtual void SaveTo (FilePath const & toPath) = 0;
	virtual std::unique_ptr<OutNamedBlock> Clone () = 0;

#if defined (PACKER_TEST)
	virtual std::string Verify (MemFileReadOnly const & file) = 0;
#endif

private:
	 std::string	_name;
};

class MemOutBlock : public OutNamedBlock
{
public:
	void Write (unsigned char const * buf, unsigned int size);
	std::unique_ptr<OutNamedBlock> Clone ();
	void SaveTo (FilePath const & toPath);

#if defined (PACKER_TEST)
	std::string Verify (MemFileReadOnly const & file);
#endif

private:
	std::vector<unsigned char>	_buf;
};

class FileOutBlock : public OutNamedBlock
{
public:
	FileOutBlock (FilePath const & path)
		: _path (path)
	{}
	FileOutBlock (FileOutBlock const & block)
		: _path (block._path)
	{}
	~FileOutBlock ();

	void Init (std::string const & newName);
	void Write (unsigned char const * buf, unsigned int size)
	{
		_file.Write (buf, size);
	}
	std::unique_ptr<OutNamedBlock> Clone ();
	void SaveTo (FilePath const & toPath);

#if defined (PACKER_TEST)
	std::string Verify (MemFileReadOnly const & file);
#endif

private:
	FilePath	_path;
	FileIo		_file;
};

class OutBlockVector
{
public:
	OutBlockVector (std::unique_ptr<OutNamedBlock> & firstBlock)
	{
		_blocks.push_back (std::move(firstBlock));
	}

	void resize (unsigned int blockCount);
	unsigned int size () { return _blocks.size (); }
    OutNamedBlock & operator [] (unsigned int i) { return *_blocks [i]; }

private:
	auto_vector<OutNamedBlock>	_blocks;
};

#endif

