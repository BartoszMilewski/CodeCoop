#if !defined (CMCSESSION_H)
#define CMCSESSION_H
//
// (c) Reliable Software 1998
//

#include <windows.h>
#include <xcmc.h>

class Session
{
public:
	Session ();
	~Session ();

	CMC_session_id GetId () const { return _session; }
	void * GetCmcFunction (char const * funcName) const;

private:
	HINSTANCE		_CMCLib;
	CMC_session_id	_session;
};

#endif
