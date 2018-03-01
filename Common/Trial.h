#if !defined (TRIAL_H)
#define TRIAL_H
//----------------------------------
// (c) Reliable Software 2005 - 2009
//----------------------------------

class Catalog;
class PathFinder;

class Trial
{
public:
	Trial (Catalog & catalog);

	void Refresh (PathFinder & pathFinder);
	void Clear () { _daysLeft = 0; }

	bool IsOn () const { return _daysLeft != 0;}
	int GetDaysLeft () const { return _daysLeft; }
	int GetTrialStart () const { return _trialStart; }

private:
	// Notice: requires update with every release preceded by beta period
	static long const RELEASE_TIME = 0x4cd1bf4a; // Nov 03, 2010
	static long const secsInDay = 24*60*60;
	static long const trialDays = 31;

	long _trialStart;
	long _daysLeft;
};

#endif
