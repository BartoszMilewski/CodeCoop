//-------------------------------------
// (c) Reliable Software 2001-2003
// ------------------------------------

#include "precompiled.h"
#include "NamedBlock.h"

#include <LightString.h>
#include <File/MemFile.h>
#include <Dbg/Assert.h>

void MemOutBlock::Write (unsigned char const * buf, unsigned int size)
{
	unsigned int end = _buf.size ();
	// Make room for the copied elements
	_buf.resize (_buf.size () + size);
	unsigned char * to = &_buf [end];
	std::copy (buf, buf + size, to);
}

std::unique_ptr<OutNamedBlock> MemOutBlock::Clone ()
{
	std::unique_ptr<OutNamedBlock> p (new MemOutBlock ());
	return p;
}

void MemOutBlock::SaveTo (FilePath const & toPath)
{
	char const * to = toPath.GetFilePath (GetName ());
	MemFileNew out (to, File::Size (_buf.size (), 0));
	std::copy (_buf.begin (), _buf.end (), out.GetBuf ());
}

#if defined (PACKER_TEST)
std::string MemOutBlock::Verify (MemFileReadOnly const & file)
{
	if (_buf.size () == file.GetSize ().Low ())
	{
		if (memcmp (&_buf [0], file.GetBuf (), file.GetSize ().Low ()) != 0)
		{
			return std::string ("Decompression failed -- file contents doesn't match.");
		}
	}
	else
	{
		Msg info;
		info << "Decompression failed -- size doesn't match\n\n";
		info << "File size: " << file.GetSize ().Low () << "; uncompressed size: " << _buf.size () << "\n\n";
		return std::string (info.c_str ());
	}
	return std::string ();
}
#endif

FileOutBlock::~FileOutBlock ()
{
	_file.Close ();	// If file is closed Close does nothing
	char const * filePath = _path.GetFilePath (GetName ());
	File::DeleteNoEx (filePath);
}

void FileOutBlock::Init (std::string const & newName)
{
	OutNamedBlock::Init (newName);
	char const * filePath = _path.GetFilePath (GetName ());
	_file.Create (filePath);
}

std::unique_ptr<OutNamedBlock> FileOutBlock::Clone ()
{
	std::unique_ptr<OutNamedBlock> p (new FileOutBlock (*this));
	return p;
}

void FileOutBlock::SaveTo (FilePath const & toPath)
{
	_file.Close ();
	char const * from = _path.GetFilePath (GetName ());
	char const * to = toPath.GetFilePath (GetName ());
	if (File::Exists (to))
		File::DeleteNoEx (to);
	if (!File::Exists (to))
		File::Move (from, to);
}

#if defined (PACKER_TEST)
std::string FileOutBlock::Verify (MemFileReadOnly const & file)
{
	MemFileReadOnly result (_path.GetFilePath (GetName ()));
	if (result.GetSize () == file.GetSize ())
	{
		if (memcmp (result.GetBuf (), file.GetBuf (), file.GetSize ().Low ()) != 0)
		{
			return std::string ("Decompression failed -- file contents doesn't match.");
		}
	}
	else
	{
		Msg info;
		info << "Decompression failed -- size doesn't match\n\n";
		info << "File size: " << file.GetSize ().Low () << "; uncompressed size: " << result.GetSize ().Low () << "\n\n";
		return std::string (info.c_str ());
	}
	return std::string ();
}
#endif

void OutBlockVector::resize (unsigned int blockCount)
{
	Assert (_blocks.size () == 1);
	OutNamedBlock * firstBlock = _blocks [0];
	for (unsigned int i = 0; i < blockCount - 1; ++i)
	{
		_blocks.push_back (firstBlock->Clone ());
	}
}
