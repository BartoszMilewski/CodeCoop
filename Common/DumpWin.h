#if !defined (DUMPWIN_H)
#define DUMPWIN_H
//----------------------------------
// (c) Reliable Software 1997 - 2002
//----------------------------------

#include "EditStyle.h"

class DumpWindow
{
public:
	virtual ~DumpWindow ()
	{}

	enum Style
	{
		styH1,
		styH2,
		styNormal,
		styCodeFirst,	// First code line style
		styCodeNext,	// Next code line style
		// must be last
		styLast
	};

	virtual void PutLine (char const * line, Style style = styNormal) {};
	virtual void PutLine (std::string const & line, Style style = styNormal) {};
	virtual void PutLine (std::string const & line, EditStyle act, bool isFirstLine = true) {};
};

#endif
