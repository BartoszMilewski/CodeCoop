#if !defined (COOPCMDLINE_H)
#define COOPCMDLINE_H
//------------------------------------
//  (c) Reliable Software, 2000 - 2006
//------------------------------------

#include "FileClassifier.h"

class CmdLine
{
public:
	CmdLine (std::string const & cmdName,
			FileListClassifier::ProjectFiles const * files);

    operator std::string const & () const { return _line; }

private:
	std::string	_line;
};

#endif
