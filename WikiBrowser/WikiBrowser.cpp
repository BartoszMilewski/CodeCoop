//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "WikiBrowser.h"
#include "Global.h"
#include "Commander.h"
#include "UiManager.h"
#include "AccelTab.h"
#include "OutputSink.h"

#include <Win/MsgLoop.h>
#include <Ctrl/Accelerator.h>

const char Wiki::BrowserCtrl::CLASS_NAME [] = "Reliable Software Wiki Browser";
const char Wiki::BrowserCtrl::PrefsKeyName [] = "Wiki Browser";

Wiki::BrowserCtrl::BrowserCtrl (Win::MessagePrepro & msgPump)
	: _msgPump (msgPump),
	  _msgCommand (UM_COOP_COMMAND)
{}

Wiki::BrowserCtrl::~BrowserCtrl ()
{}

Notify::Handler * Wiki::BrowserCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_uiManager.get ())
		return _uiManager->GetNotifyHandler (winFrom, idFrom);
	return 0;
}

bool Wiki::BrowserCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		_commander.reset (new Commander (_h));
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		_uiManager.reset (new UiManager (_h, *_cmdVector, *this));
		_uiManager->AttachMenu2Window (_h);
		Accel::Maker accelMaker (Accel::Keys, *_cmdVector);
		_kbdAccel.reset (new Accel::Handler (_h, accelMaker.Create ()));
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

bool Wiki::BrowserCtrl::OnDestroy () throw ()
{
	_uiManager->OnDestroy ();
	// Destroy UI manager, but first zero the unique_ptr
	UiManager * tmp = _uiManager.release ();
	delete tmp;
	Win::Quit ();
	return true;
}

bool Wiki::BrowserCtrl::OnClose () throw ()
{
	_h.Destroy ();
	return true;
}

bool Wiki::BrowserCtrl::OnShutdown (bool isEndOfSession, bool isLoggingOff) throw ()
{
	_h.Destroy ();
	return true;
}

bool Wiki::BrowserCtrl::OnSize (int width, int height, int flag) throw ()
{
	_uiManager->OnSize (width, height); 
	return true;
}

bool Wiki::BrowserCtrl::OnFocus (Win::Dow::Handle winPrev) throw ()
{
	_uiManager->OnFocus (winPrev);
	return true;
}

bool Wiki::BrowserCtrl::OnControl (Win::Dow::Handle control, int id, int notifyCode) throw ()
{
	return true;
}

bool Wiki::BrowserCtrl::OnKillFocus (Win::Dow::Handle winNext) throw ()
{
	return true;
}

bool Wiki::BrowserCtrl::OnSettingChange (Win::SystemWideFlags flags, char const * sectionName) throw ()
{
	_h.ForceRepaint ();
	return true;
}

bool Wiki::BrowserCtrl::OnInitPopup (Menu::Handle menu, int pos) throw ()
{
	try
	{
		_uiManager->RefreshPopup (menu, pos);
	}
	catch (...) 
	{
		Win::ClearError ();
		return false;
	}
	return true;
}

bool Wiki::BrowserCtrl::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
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

bool Wiki::BrowserCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (isAccel)
	{
		// Keyboard accelerators to invisible or disabled menu items are not executed
		if (_cmdVector->Test (cmdId) != Cmd::Enabled)
			return true;
	}
	MenuCommand (cmdId);
	return true;
}

bool Wiki::BrowserCtrl::IsEnabled (std::string const & cmdName) const throw ()
{
	return (_cmdVector->Test (cmdName.c_str ()) == Cmd::Enabled);
}

void Wiki::BrowserCtrl::ExecuteCommand (std::string const & cmdName) throw ()
{
	if (IsEnabled (cmdName))
	{
		int cmdId = _cmdVector->Cmd2Id (cmdName.c_str ());
		Assert (cmdId != -1);
		MenuCommand (cmdId);
	}
}

void Wiki::BrowserCtrl::ExecuteCommand (int cmdId) throw ()
{
	Assert (cmdId != -1);
	MenuCommand (cmdId);
}

// The name of the command is pushed on the queue, followed by pairs of (argName, argValue)
// Command message is then posted; argument count passed in wParam
void Wiki::BrowserCtrl::PostCommand (std::string const & cmdName, NamedValues const & namedArgs)
{
	bool addCmd = true;
	unsigned count = namedArgs.size ();
	if (_cmdQueue.size () != 0)
	{
		// Check if we already have this command in the queue
		CmdData cmdData = _cmdQueue.front ();
		if (cmdData.first == cmdName)
		{
			// Command names match - check argument list
			ArgList const & argList = cmdData.second;
			if (count == argList.size ())
			{
				// The same argument count
				ArgList::const_iterator iter = argList.begin ();
				for ( ; iter != argList.end (); ++iter)
				{
					std::string argValue = namedArgs.GetValue (iter->first);
					if (argValue != iter->second)
					{
						// Different argument values - add new command to the queue
						break;
					}
				}
				if (iter == argList.end ())
					addCmd = false;	// Identical commands - don't add to the queue
			}
		}
	}

	if (addCmd)
	{
		ArgList args;
		for (NamedValues::Iterator it = namedArgs.begin (); it != namedArgs.end (); ++it)
		{
			args.push_back (std::make_pair (it->first, it->second));
		}
		_cmdQueue.push_back (std::make_pair (cmdName, args));
		_h.PostMsg (_msgCommand);
	}
}

void Wiki::BrowserCtrl::DisableKeyboardAccelerators (Win::Accelerator * accel)
{
	_msgPump.SetKbdAccelerator (accel);
}

void Wiki::BrowserCtrl::EnableKeyboardAccelerators ()
{
	_msgPump.SetKbdAccelerator (_kbdAccel.get ());
}

void Wiki::BrowserCtrl::SetWindowPlacement (Win::ShowCmd cmdShow, bool multipleWindows)
{
	Assert (_uiManager.get () != 0);
	_uiManager->SetWindowPlacement (cmdShow, multipleWindows);
}

void Wiki::BrowserCtrl::MenuCommand (int cmdId)
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
}
