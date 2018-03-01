#if !defined (SIMPLESESSION_H)
#define SIMPLESESSION_H
//
// (c) Reliable Software 1998
//

#include <windows.h>
#include <mapi.h>

class Session
{
public:
	Session ();
	~Session ();

	LHANDLE GetHandle () const { return _session; }
	void * GetFunction (char const * funcName) const;

private:
	HINSTANCE	_SimpleMapiLib;
	LHANDLE		_session;
};

#endif
