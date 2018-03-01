//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "CoopBlocker.h"
#include "Messengers.h"
#include "Global.h"
#include "DispatcherProxy.h"
#include "OutputSink.h"

#include <Ctrl/ProgressMeter.h>

class CoopBlockerUser
{
public:
	CoopBlockerUser (CoopBlocker & blocker)
		: _blocker (blocker),
		  _holdValid (false)
	{
		_blocker.Hold ();
	}
	~CoopBlockerUser ()
	{
		if (!_holdValid)
			_blocker.Resume ();
	}

	void Accept () { _holdValid = true; }

private:
	CoopBlocker &	_blocker;
	bool			_holdValid;
};

bool CoopBlocker::TryToHold (bool quiet) throw ()
{
	CoopBlockerUser coopHold (*this);
	try
	{
		// Block Code Co-op operations
		if (GUIInstancesPresent ())
		{
			if (!quiet)
				TheOutput.Display ("Please, close other instances of Code Co-op before proceeding.",
								   Out::Error,
								   _topWin);
			return false;
		}
		if (!CloseServerInstances ())
		{
			if (!quiet)
				TheOutput.Display ("There are still instances of Code Co-op running in the background.\n"
								   "Use Windows Task Manager to close them, before proceeding.",
								   Out::Error,
								   _topWin);
			return false;
		}
		coopHold.Accept ();
		return true;
	}
	catch (Win::Exception ex)
	{
		if (!quiet)
			TheOutput.DisplayException (ex, _topWin);
	}
	catch ( ... )
	{
		if (!quiet)
			TheOutput.Display ("Unknown exception during backup.", Out::Error, _topWin);
	}

	return false;
}

void CoopBlocker::Resume ()
{
	try
	{
		_sccProxy.SccResume ();
		DispatcherProxy dispatcher;
	}
	catch ( ... )
	{
		TheOutput.Display ("Cannot restart Code Co-op. Please, close all applications that might be using Code Co-op\n"
						   "and restart Code Co-op manually.");
	}
}

bool CoopBlocker::GUIInstancesPresent ()
{
	CoopDetector detector (_topWin);
	Win::EnumProcess (CoopClassName, detector);
	return detector.MoreCoopInstancesDetected ();
}

// Returns true when no server instances running
bool CoopBlocker::CloseServerInstances ()
{
	// REVISIT: implementation - use ProcessGroup class to kill a group of preocesses
	int const OneSecond = 1000;
	bool result = true;
	BackupNotify backup (_topWin);
	Win::EnumProcess (ServerClassName, backup);
	if (backup.NotificationSent ())
	{
		_meter.SetRange (0, 10, 1);
		int count = 10;
		while (count > 0)
		{
			_meter.StepIt ();
			::Sleep (OneSecond);
			--count;
		}
		_meter.Close ();
		CoopDetector detector (_topWin);
		Win::EnumProcess (ServerClassName, detector);
		result = !detector.MoreCoopInstancesDetected ();
	}
	return result;
}

void CoopBlocker::Hold ()
{
	DispatcherProxy dispatcher;
	dispatcher.Kill ();
	_sccProxy.SccHold ();
}
