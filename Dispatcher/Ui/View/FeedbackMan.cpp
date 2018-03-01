// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------
#include "precompiled.h"
#include "FeedbackMan.h"
#include "Global.h"
#include "DispatcherMsg.h"
#include "DispatcherParams.h"
#include "resource.h"
#include <StringOp.h>


FeedbackMan TheFeedbackMan;

FeedbackMan::FeedbackMan ()
: _appWin (0),
  _inCount (0),
  _outCount (0),
  _msgActivityChange (UM_ACTIVITY_CHANGE)
{}

void FeedbackMan::Init (Win::Dow::Handle appWin)
{
	_appWin = appWin;
	_taskbarIcon.reset (new TaskbarIcon (
				_appWin, 
				TASKBAR_ICON_ID, 
				_noScriptsIcon, 
				Win::UserMessage (UM_TASKBAR_ICON_NOTIFY)));

	Icon::SharedMaker icon;
	icon.SetSize (16, 16);
	_alertNoScriptsIcon    = icon.Load (_appWin.GetInstance (), ID_ALERT_NO);
	_alertInScriptsIcon    = icon.Load (_appWin.GetInstance (), ID_ALERT_IN);
	_alertOutScriptsIcon   = icon.Load (_appWin.GetInstance (), ID_ALERT_OUT);
	_alertInOutScriptsIcon = icon.Load (_appWin.GetInstance (), ID_ALERT_WAITING);
	_alertActivityIcon     = icon.Load (_appWin.GetInstance (), ID_ALERT_ACTIVITY);
	_errorIcon		  = icon.Load (_appWin.GetInstance (), ID_ERROR);
	_noScriptsIcon    = icon.Load (_appWin.GetInstance (), ID_NOSCRIPTS);
	_inScriptsIcon    = icon.Load (_appWin.GetInstance (), ID_IN_SCRIPTS);
	_outScriptsIcon   = icon.Load (_appWin.GetInstance (), ID_OUT_SCRIPTS);
	_inOutScriptsIcon = icon.Load (_appWin.GetInstance (), ID_WAITING_SCRIPTS);

	ShowState ();
}

void FeedbackMan::ShowState ()
{
	std::string tip (DispatcherTitle);
	tip += '\n';
	DispatchingActivity currentActivity = _activity.Get ();
	if (currentActivity != Idle)
	{
		switch (currentActivity)
		{
		case RetrievingEmail:
			tip += "Checking e-mail for incoming scripts.";
			break;
		case Forwarding:
			tip += "Copying scripts over Local Area Network.";
			break;
		case SendingEmail:
			tip += "Sending scripts by e-mail.";
			break;
		};
		_taskbarIcon->SetImage (_alertActivityIcon);
	}
	else if (_updateInfo.empty ())
	{
		if (_inCount == 0 && _outCount == 0)
		{
			tip += "No pending scripts";
			_taskbarIcon->SetImage (_noScriptsIcon);
		}
		else
		{
			if (_inCount != 0)
			{
				tip += ToString (_inCount);
				tip += " project(s) with pending scripts";
				_taskbarIcon->SetImage (_inScriptsIcon);
			}

			if (_outCount != 0)
			{
				if (_inCount != 0)
				{
					tip += '\n';
					_taskbarIcon->SetImage (_inOutScriptsIcon);
				}
				else
				{
					_taskbarIcon->SetImage (_outScriptsIcon);
				}
				tip += ToString (_outCount);
				tip += " script(s) in Public Inbox";
			}
		}
	}
	else
	{
		tip += _updateInfo;
		tip += "\nDouble click to see details.";
		if (_inCount == 0 && _outCount == 0)
		{
			_taskbarIcon->SetImage (_alertNoScriptsIcon);
		}
		else if (_inCount != 0 && _outCount == 0)
		{
			_taskbarIcon->SetImage (_alertInScriptsIcon);
		}
		else if (_inCount == 0)
		{
			_taskbarIcon->SetImage (_alertOutScriptsIcon);
		}
		else
		{
			_taskbarIcon->SetImage (_alertInOutScriptsIcon);
		}
	}
	_taskbarIcon->SetToolTip (tip.c_str ());
}

void FeedbackMan::DisplayAlert (std::string const & alert)
{
	_taskbarIcon->SetBalloonTip (alert.c_str (), DispatcherTitle);
}

void FeedbackMan::SetUpdateReady (char const * info)
{
	Assert (info != 0);
	_updateInfo.assign (info);
	
	ShowState ();
}

void FeedbackMan::ResetUpdateReady ()
{
	_updateInfo.clear ();
	ShowState ();
}

void FeedbackMan::ShowActivity ()
{
	ShowState ();
}

void FeedbackMan::Animate ()
{
	if (!IsDispatchingActivity ())
		return;
	// Revisit: implement
	// toggle displayed icon
	// when activity ends, ShowState will display corresponding icon, 
	// so there is no need for separate StopAnimate method
}
