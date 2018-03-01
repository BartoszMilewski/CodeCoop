//------------------------------------
//  (c) Reliable Software, 2002 - 2005
//------------------------------------

#include "precompiled.h"
#include "DisplayFrameCtrl.h"
#include "OutputSink.h"
#include "Registry.h"

#include <Win/Message.h>

DisplayFrameController::DisplayFrameController (bool addScrollBars)
	: Notify::RichEditHandler (DisplayId),
	  _addScrollBars (addScrollBars)
{}

Notify::Handler * DisplayFrameController::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (IsHandlerFor (idFrom))
		return this;
	else
		return 0;
}

bool DisplayFrameController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		// Create rich edit control
		if (_addScrollBars)
			_display.AddScrollBars ();
		_display.Create (_h, DisplayId);
		success = true;
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
		success = false;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Display window initialization -- unknown error", Out::Error);
		success = false;
	}
	return true;
}

bool DisplayFrameController::OnDestroy () throw ()
{
	if (IsSaveUIPrefs ())
	{
		// Remember display frame widow position and size
		Win::Placement placement (_h);
		// Revisit: currently we use predefined registry key 'List Window' -- allow key selection
		Registry::UserPreferences preferences;
		preferences.SaveListWinPlacement (placement);
	}
	// Destroy display pane
	_display.Destroy ();
	return true;
}

bool DisplayFrameController::OnClose () throw ()
{
	_h.Destroy ();
	return true;
}

bool DisplayFrameController::OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ()
{
	_h.Destroy ();
	return true;
}

bool DisplayFrameController::OnSize (int width, int height, int flag) throw ()
{
	_display.Move (0, 0, width, height);
	return true;
}

bool DisplayFrameController::OnControl (Win::Dow::Handle control, unsigned id, unsigned notifyCode) throw ()
{
	if (id == DisplayId && Win::Edit::IsKillFocus (notifyCode) && IsCloseOnLostFocus ())
		_h.PostMsg (Win::CloseMessage ());
	return true;
}

bool DisplayFrameController::OnKillFocus (Win::Dow::Handle winNext) throw ()
{
	if (_display != winNext && IsCloseOnLostFocus ())
		_h.Destroy ();
	return true;
}

bool DisplayFrameController::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	if (flags.IsNonClientMetrics ())
	{
		_display.RefreshFormats ();
		_h.ForceRepaint ();
		return true;
	}
	return false;
}

bool DisplayFrameController::OnRequestResize (Win::Rect const & rect) throw ()
{
	_sizeRequest = rect;
	return true;
}
