#if !defined (FILEIO_H)
#define FILEIO_H
//----------------------------------
// (c) Reliable Software 1998 - 2008
//----------------------------------

#include "File.h"

#include <fstream>

// Standard file i/o interface
class FileIo : public File
{
public:
	FileIo () {}
	FileIo (std::string const & path, 
			File::Mode mode, 
			File::Attributes attrib = File::NormalAttributes ())
		: File (path, mode, attrib)
	{}
	FileIo (File::File_Ref ref)
		: File (ref)
	{}
	File::Offset GetPosition () const;
	File::Offset SetPosition (File::Offset pos);
	void	Read (void * buf, unsigned long size);
	void	Write (void const * buf, unsigned long size);
	void	FillBuf (void * buf, unsigned long & size);
};

class OutStream : public std::ofstream
{
public:
	OutStream () {}
	OutStream (std::string const & path, std::ios_base::open_mode openMode = std::ios_base::out);
	void Open (std::string const & path, std::ios_base::open_mode openMode = std::ios_base::out);
};

class InStream : public std::ifstream
{
public:
	InStream (std::string const & path, std::ios_base::open_mode openMode = std::ios_base::in);
};

class InOutStream : public std::fstream
{
    void _Add_vtordisp1() { } // Required to avoid VC++ warning C4250
    void _Add_vtordisp2() { } // Required to avoid VC++ warning C4250
public:
	InOutStream (std::string const & path, 
		std::ios_base::open_mode openMode = (std::ios_base::in | std::ios_base::out));
};

#endif
