//-----------------------------------
// (c) Reliable Software, 2004 - 2008
//-----------------------------------

#include "precompiled.h"
#include "TopCtrl.h"
#include "OutSink.h"
#include "WinTestRegistry.h"
#include "Resource/resource.h"
#include "Commander.h"
#include "AccelTable.h"
#include "Menu.h"
#include "View.h"
#include "Modeless.h"

#include <Sys/WinString.h>
#include <Win/MsgLoop.h>

TopCtrl::TopCtrl (char const * cmdLine, Win::MessagePrepro & msgPrepro)
	: _initMsg ("InitMessage"),
	  _fileDropMsg ("FileDropMessage"),
	  _msgPrepro (msgPrepro),
	  _ready (false),
	  _cmdLine (cmdLine),
	  _modelessMan (msgPrepro)
{}

TopCtrl::~TopCtrl ()
{}

bool TopCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	Win::Dow::Handle win = GetWindow ();
	ResString caption (win.GetInstance (), ID_CAPTION);
	try
	{
		TheOutput.Init (caption);
		win.SetText (caption);
		_commander.reset (new Commander (win));
		_commander->SetModelessManager (&_modelessMan);
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		Accel::Maker accelMaker (Accel::Keys, *_cmdVector);
		_kbdAccel.reset (new Accel::Handler (win, accelMaker.Create ()));
		_msgPrepro.SetKbdAccelerator (_kbdAccel.get ());
		_menu.reset (new Menu::DropDown (Menu::barItems, *_cmdVector));
		_menu->AttachToWindow (win);
		_view.reset (new View (win));
		RegisterAsDropTarget(win);

		win.PostMsg (_initMsg);
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
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error);
		success = false;
	}
	TheOutput.SetParent (win);
	_ready = true;
	return true;
}

void TopCtrl::OnInit ()
{
	// Window is ready
	// Put initialization code here
}

bool TopCtrl::OnDestroy () throw ()
{
	try
	{
		Win::Placement placement (GetWindow ());
		TheRegistry.SavePlacement (placement);
	}
	catch (...)
	{}
	Win::Quit ();
	return true;
}

bool TopCtrl::OnRegisteredMessage (Win::Message & msg) throw ()
{
	if (msg == _initMsg)
	{
		OnInit ();
		return true;
	}
	else if (msg == _fileDropMsg)
	{
		Win::FileDropOwner droppedFiles (msg.GetWParam ());
		std::string msg ("You have fropped the following file(s):\n\n");
		for (Win::FileDropHandle::Sequencer seq (droppedFiles); !seq.AtEnd (); seq.Advance ())
		{
			msg += seq.GetFilePath ();
			msg += "\n";
		}
		TheOutput.Display (msg.c_str ());
		return true;
	}

	return false;
}

bool TopCtrl::OnSize (int width, int height, int flag) throw ()
{
	if (_ready)
		_view->Size (width, height); 
	return true;
}

bool TopCtrl::OnInitPopup (Menu::Handle menu, int pos) throw ()
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

bool TopCtrl::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
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

Notify::Handler * TopCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	return 0;
}

bool TopCtrl::OnCommand (int cmdId, bool isAccel) throw ()
{
	if (isAccel)
	{
		// Revisit: maybe we should have Test method that takes cmdId?
		// Keyboard accelerators to invisible or disabled menu items are not executed
		char const * name = _cmdVector->GetName (cmdId);
		if (_cmdVector->Test (name) != Cmd::Enabled)
			return true;
	}
	MenuCommand (cmdId);
	return true;
}

void TopCtrl::MenuCommand (int cmdId)
{
	Win::ClearError ();
	try
	{
		_cmdVector->Execute (cmdId);
	}
	catch (Win::ExitException e)
	{
		TheOutput.Display (e);
		_h.Destroy ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (std::bad_alloc bad)
	{
		Win::ClearError ();
		TheOutput.Display ("Operation aborted: not enough memory.");
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal Error: Command execution failure", Out::Error);
	}
}

void TopCtrl::OnFileDrop (Win::FileDropHandle droppedFiles, Win::Point dropPoint)
{
	_fileDropMsg.SetWParam (reinterpret_cast<unsigned int>(droppedFiles.ToNative ()));
	_h.PostMsg (_fileDropMsg);
}

