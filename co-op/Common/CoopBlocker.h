#if !defined (COOPBLOCKER_H)
#define COOPBLOCKER_H
//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "SccProxyEx.h"

namespace Progress { class Meter; }

class CoopBlocker
{
	friend class CoopBlockerUser;

public:
	CoopBlocker (Progress::Meter & meter, Win::Dow::Handle topWin)
		: _meter (meter),
		  _topWin (topWin)
	{}
	~CoopBlocker ()
	{
		Resume ();
	}

	bool TryToHold (bool quiet = false) throw ();
	void Resume ();

private:
	bool GUIInstancesPresent ();
	bool CloseServerInstances ();
	void Hold ();

private:
	SccProxyEx			_sccProxy;
	Progress::Meter &	_meter;
	Win::Dow::Handle	_topWin;
};

#endif
