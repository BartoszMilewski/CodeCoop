#if !defined (WINOUTPUT_H)
#define WINOUTPUT_H
// (c) Reliable Software 2003
#include <Win/Output.h>

class WinOut
{
public:
	virtual ~WinOut () {}
	// Only call from main thread
	virtual void PutLine (char const * text) = 0;
	// Call from any thread
	virtual void PostLine (char const * text) {}
};

// Message Box display
extern Out::Sink TheOutput;

#endif
