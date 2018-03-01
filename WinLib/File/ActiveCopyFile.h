#if !defined (ACTIVECOPYFILE_H)
#define ACTIVECOPYFILE_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------

#include "File.h"

class ActiveCopyFile
{
public:
	virtual ~ActiveCopyFile () {}

	virtual void OpenRead () = 0;
	virtual void OpenWrite () = 0;
	virtual ::File::Size GetSize () = 0;
	virtual std::string const & GetFilePath () const = 0;
	virtual void Read (char * buf, unsigned long & size) = 0;
	virtual void Write (char const * buf, unsigned long size) = 0;
	virtual void Commit () = 0;
};

#endif
