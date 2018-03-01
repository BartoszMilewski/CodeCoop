//------------------------------------
//  (c) Reliable Software, 2003 - 2006
//------------------------------------

#include "precompiled.h"
#include "DbgMonitorCtrl.h"
#include "MenuTable.h"
#include "Commander.h"
#include "OutputSink.h"
#include "Registry.h"
#include "Global.h"

#include <Dbg/Monitor.h>
#include <Win/EnumProcess.h>

class DbgMonStartNotify : public Win::Messenger
{
public:

	void DeliverMessage (Win::Dow::Handle targetWindow);
};

void DbgMonStartNotify::DeliverMessage (Win::Dow::Handle targetWindow)
{
	Win::RegisteredMessage msg (Dbg::Monitor::UM_DBG_MON_START);
	targetWindow.PostMsg (msg);
}

class DbgMonStopNotify : public Win::Messenger
{
public:

	void DeliverMessage (Win::Dow::Handle targetWindow);
};

void DbgMonStopNotify::DeliverMessage (Win::Dow::Handle targetWindow)
{
	Win::RegisteredMessage msg (Dbg::Monitor::UM_DBG_MON_STOP);
	targetWindow.PostMsg (msg);
}

DbgMonitorCtrl::DbgMonitorCtrl (bool addScrollBars)
	: Notify::RichEditHandler (DisplayId),
	  _msgPutLine (Dbg::Monitor::UM_PUT_LINE)
{
	if (addScrollBars)
		AddScrollBars ();
	SaveUIPrefs ();
}

DbgMonitorCtrl::~DbgMonitorCtrl ()
{}

Notify::Handler * DbgMonitorCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (IsHandlerFor (idFrom))
		return this;
	else
		return 0;
}

bool DbgMonitorCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		// Create rich edit control
		if (IsAddScrollBars ())
			_display.AddScrollBars ();
		_display.Create (_h, DisplayId);
		if (IsSaveUIPrefs ())
		{
			// Restore display frame widow position and size
			Win::Placement placement;
			Registry::UserPreferences preferences;
			preferences.GetWinPlacement ("Dbg Monitor", placement);
			_h.SetPlacement (placement);
		}
		_commander.reset (new Commander (_h, _display));
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		_menu.reset (new Menu::DropDown (Menu::barItems, *_cmdVector));
		_menu->AttachToWindow (_h);
		DbgMonStartNotify notify;
		Win::EnumProcess (CoopClassName, notify);
		Win::EnumProcess (ServerClassName, notify);
		Win::EnumProcess (DispatcherClassName, notify);
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

bool DbgMonitorCtrl::OnDestroy () throw ()
{
	DbgMonStopNotify notify;
	Win::EnumProcess (CoopClassName, notify);
	Win::EnumProcess (ServerClassName, notify);
	Win::EnumProcess (DispatcherClassName, notify);
	if (IsSaveUIPrefs ())
	{
		// Remember display frame widow position and size
		Win::Placement placement (_h);
		Registry::UserPreferences preferences;
		preferences.SaveWinPlacement ("Dbg Monitor", placement);
	}
	// Destroy display pane
	_display.Destroy ();
	Win::Quit ();
	return true;
}

bool DbgMonitorCtrl::OnClose () throw ()
{
	_h.Destroy ();
	return true;
}

bool DbgMonitorCtrl::OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ()
{
	_h.Destroy ();
	return true;
}

bool DbgMonitorCtrl::OnSize (int width, int height, int flag) throw ()
{
	_display.Move (0, 0, width, height);
	return true;
}

bool DbgMonitorCtrl::OnControl (Win::Dow::Handle control, int id, int notifyCode) throw ()
{
	if (id == DisplayId && Win::Edit::IsKillFocus (notifyCode) && IsCloseOnLostFocus ())
		_h.PostMsg (Win::CloseMessage ());
	return true;
}

bool DbgMonitorCtrl::OnKillFocus (Win::Dow::Handle winNext) throw ()
{
	if (_display != winNext && IsCloseOnLostFocus ())
		_h.Destroy ();
	return true;
}

bool DbgMonitorCtrl::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	if (flags.IsNonClientMetrics ())
	{
		_display.RefreshFormats ();
		_h.ForceRepaint ();
		return true;
	}
	return false;
}

bool DbgMonitorCtrl::OnRequestResize (Win::Rect const & rect) throw ()
{
	_sizeRequest = rect;
	return true;
}

bool DbgMonitorCtrl::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	try
	{
		_menu->RefreshPopup (menu, pos);
	}
	catch (...) 
	{
		Win::ClearError ();
		return false;
	}
	return true;
}

bool DbgMonitorCtrl::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
	}
	else
	{
		return false;
	}
	return true;
}

bool DbgMonitorCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	try
	{
		_cmdVector->Execute (cmdId);
	}
	catch (Win::ExitException )
	{
		_h.Destroy ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Command execution failure", Out::Error);
	}
	return true;
}

bool DbgMonitorCtrl::OnRegisteredMessage (Win::Message & msg) throw ()
{
	return false;
}

bool DbgMonitorCtrl::OnInterprocessPackage (unsigned int msg, char const * package, unsigned int errCode, long & result) throw ()
{
	if (_msgPutLine == msg)
	{
		if (errCode != 0)
		{
			Assert (!"Debug Output Monitor -- debug package cannot be received");
			result = 1;
		}
		else
		{
			OnDbgOutput (package);
			result = 0;
		}
		return true;
	}
	return false;
}

void DbgMonitorCtrl::OnDbgOutput (char const * buf)
{
	Dbg::Monitor::Msg msg (buf);
	if (msg.IsBogus ())
		return;

	DumpWindow::Style style = msg.IsHeader () ? DumpWindow::styH2 : DumpWindow::styNormal;
	char const * outLine = msg.c_str ();
	if (outLine != 0)
	{
		std::string threadId (msg.GetThreadId ());
		_display.SetTextColor (_colorMap.GetColor (threadId));
		_display.PutLine (outLine, style);
	}
}

DbgMonitorCtrl::ColorMap::ColorMap ()
	: _firstAvailable (0)
{
	_colors.push_back (Win::Color (0,    0,    0));		// Black
	_colors.push_back (Win::Color (0,    0,    0xff));	// Blue
	_colors.push_back (Win::Color (0xff, 0,    0));		// Red
	_colors.push_back (Win::Color (0xff, 0,    0xff));	// Magenta
	_colors.push_back (Win::Color (0xff, 0x80, 0x40));	// Orange
	_colors.push_back (Win::Color (0x40, 0x80, 0x40));	// Green
}

Win::Color DbgMonitorCtrl::ColorMap::GetColor (std::string const & threadId)
{
	std::map<std::string, unsigned>::iterator iter = _map.find (threadId);
	if (iter != _map.end ())
	{
		// Thread already has assigned color
		return _colors [iter->second];
	}
	else
	{
		// Assign color to the new thread
		Win::Color newColor = _colors [_firstAvailable];
		_map.insert (std::make_pair (threadId, _firstAvailable));
		_firstAvailable = (_firstAvailable + 1) % _colors.size ();
		return newColor;
	}
}
