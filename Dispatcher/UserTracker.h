#if !defined (USERTRACKER_H)
#define USERTRACKER_H
// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------

#include <Sys/Dll.h>

class UserTracker
{
	typedef BOOL (WINAPI * GetLastInputInfoPtr) (OUT LASTINPUTINFO * plii);
public:
	UserTracker ();
	bool CanTrack () const { return _canTrack; }
	bool HasBeenBusy (unsigned int withinPastNumberOfSeconds = 10);
private:
	Dll					_user32;
	GetLastInputInfoPtr _getLastInputInfoPtr;
	bool				_canTrack;
};

#endif
