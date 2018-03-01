// ---------------------------
// (c) Reliable Software, 2004
// ---------------------------
#include "precompiled.h"
#include "UserTracker.h"
#include <Sys/SysVer.h>

UserTracker::UserTracker ()
: _user32 ("user32.dll"),
  _canTrack (false)
{
	SystemVersion sysVer;
	_canTrack = sysVer.MajorVer () >= 5;
	if (_canTrack)
		_user32.GetFunction ("GetLastInputInfo", _getLastInputInfoPtr);
}

bool UserTracker::HasBeenBusy (unsigned int withinPastNumberOfSeconds)
{
	Assert (_canTrack);

	LASTINPUTINFO info;
	info.cbSize = sizeof (LASTINPUTINFO);
	_getLastInputInfoPtr (&info); 

	return ::GetTickCount () - info.dwTime < 1000 * withinPastNumberOfSeconds;
}

namespace UnitTest
{
	void UserTracking ()
	{
		UserTracker tracker;
		bool canTrack = tracker.CanTrack ();
		bool busy = false;
		if (canTrack)
			busy = tracker.HasBeenBusy ();
	}
}
