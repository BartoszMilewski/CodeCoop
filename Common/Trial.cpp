//----------------------------------
// (c) Reliable Software 2005 - 2006
//----------------------------------

#include "precompiled.h"
#include "Trial.h"
#include "Catalog.h"
#include "PathFind.h"

#include <TimeStamp.h>

// Fresh install: 
//    o Set _trialStart to current time. _trialStart > RELEASE_TIME
//    o First visit: project not stamped, stamp with _trialStart
//    o Subsequent visits: projectStamp == _trialStart
// Re-installation with fresh RELEASE_TIME: _trialStart < RELEASE_TIME
//    o Set _trialStart to current time. _trialStart > RELEASE_TIME
//    o First visit: projectStamp < _trialStart, _trialStart > RELEASE_TIME. Stamp project with _trialStart
//    o Subsequent visits: projectStamp == _trialStart
// Re-installation with the same RELEASE_TIME. _trialStart > RELEASE_TIME
//    o First and subsequent visits as above

Trial::Trial (Catalog & catalog)
	: _trialStart (0),
	  _daysLeft (0)
{
#if 0
	// Use for generating a new value for RELEASE_TIME
	long newTrialStart = CurrentTime ();
#endif

	bool firstRun = !catalog.GetTrialStart (_trialStart);
	if (firstRun)
	{
		// Store in the catalog the date of first run on this machine
		_trialStart = CurrentTime ();
		catalog.SetTrialStart (_trialStart);
	}
	else if (_trialStart < RELEASE_TIME)
	{
		// Restart trial
		_trialStart = CurrentTime ();
		catalog.SetTrialStart (_trialStart);
	}

#if defined BETA
	// During Beta unlicensed user has always trial days left > 0.
	// The time bomb will hit him
	_daysLeft = trialDays;
#else
	int daysLeft =
		trialDays - ((CurrentTime () - _trialStart) / secsInDay);
	if (daysLeft > 0)
		_daysLeft = daysLeft;
#endif
}

void Trial::Refresh (PathFinder & pathFinder)
{
	Assert(_trialStart >= RELEASE_TIME);
	if (!pathFinder.IsLockStamped ())
	{
		// First time visit. Mark the project.
		// Could also happen if the project was created during Beta period.
		pathFinder.StampLockFile (_trialStart);
		return;
	}

	if (pathFinder.IsStampEqual (_trialStart))
		return;

	if (pathFinder.IsStampLess (_trialStart))
	{
		// projectStamp < _trialStart
		pathFinder.StampLockFile (_trialStart);
		return;
	}

	// We have a cheater: projectStamp > _trialStart
	_daysLeft = 0;
}
