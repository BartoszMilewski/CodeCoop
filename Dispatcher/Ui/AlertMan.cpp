//----------------------------------
// (c) Reliable Software 2002 - 2006
//----------------------------------

#include "precompiled.h"

#include "AlertMan.h"
#include "FeedbackMan.h"
#include "AlertLog.h"
#include "Global.h"
#include "resource.h"
#include <Ctrl/Output.h>

AlertMan TheAlertMan;

AlertMan::AlertMan ()
	: _msgDisplayAlert (UM_DISPLAY_ALERT),
	  _alertLog (0)
{}

// policy of displaying alerts:
// displayed are:
//	- all normal-priority alerts
//	- low-priority alerts in verbose mode

void AlertMan::PostQuarantineAlert (std::string const & msg)
{
	Assert (!msg.empty ());
	AlertMsg alert (Quarantine, msg, true, false); // verbose, not low priority
	AddNotify (alert);
}

void AlertMan::PostUpdateAlert (std::string const & msg)
{
	Assert (!msg.empty ());
	AlertMsg alert (Update, msg, true, false); // verbose, not low priority
	AddNotify (alert);
}

void AlertMan::PostInfoAlert (
		std::string const & msg, 
		bool isVerbose, 
		bool isLowPriority,
		std::string const & hint)
{
#if 0
	// For debugging purposes
	isVerbose = true;
#endif

	Assert (!msg.empty ());
	std::string formattedMsg (msg);
	if (!hint.empty ())
	{
		formattedMsg += "\n\n";
		formattedMsg += hint;
	}
	AlertMsg alert (Info, formattedMsg, isVerbose, isLowPriority);
	AddNotify (alert);
}

void AlertMan::PostInfoAlert (Win::Exception const & e, bool isVerbose) throw ()
{
#if 0
	// For debugging purposes
	isVerbose = true;
#endif
	try
	{
		if (e.GetMessage () == 0)
			return;		// quiet

		std::string msg = Out::Sink::FormatExceptionMsg (e);
		Assert (!msg.empty ());
		AlertMsg alert (Info, msg, isVerbose, false); // not low priority
		AddNotify (alert);
	}
	catch (...)
	{}
}

void AlertMan::AddNotify (AlertMsg const & alert)
{
	_requestedAlerts.PushBack (alert);
	_win.PostMsg (_msgDisplayAlert);
}

void AlertMan::Display ()
{
	AlertMsg msg;
	while (_requestedAlerts.PopFront (msg))
	{
		Assert (msg._type != Unknown);
		Assert (_alertLog != 0);
		dbg << msg._msg.c_str () << std::endl;
		if (msg._isVerbose)
		{
			if (msg._type == Info)
				_alertLog->Add (msg._msg);

			TheFeedbackMan.DisplayAlert (msg._msg);
			_visibleAlerts.push (TypePriorityPair (msg._type, msg._isLowPriority));
		}
	}
}

AlertMan::AlertType AlertMan::PopAlert (bool & isLowPriority)
{
	Assert (IsAlerting ());
	TypePriorityPair front = _visibleAlerts.front ();
	_visibleAlerts.pop ();
	isLowPriority = front.second;
	return front.first;
}
