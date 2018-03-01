#if !defined (WINOUTPUT_H)
#define WINOUTPUT_H
// (c) Reliable Software 2003
#include <Ctrl/Output.h>

class WinOut
{
public:
	virtual ~WinOut () {}
	virtual void PutLine (char const * text) = 0;
	virtual void PutBoldLine (char const * text) = 0;
	virtual void PutMultiLine (std::string const & text)
	{
		PutLine (text.c_str ());
	}
};

extern Out::Sink TheOutput;

#endif
