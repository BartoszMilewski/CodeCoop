//---------------------------------------------------
//  (c) Reliable Software, 2001 -- 2003
//---------------------------------------------------

#include "precompiled.h"
#include "TestCtrl.h"
#include "Model.h"
#include "Commander.h"
#include "ViewMan.h"
#include "Prompter.h"
#include "OutputSink.h"

#include <Win/MsgLoop.h>
#include <Ex/WinEx.h>

TestController::TestController (Win::MessagePrepro * msgPrepro)
	: _msgPrepro (msgPrepro)
{}

TestController::~TestController ()
{}

bool TestController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	try
	{
		ThePrompter.SetWindow (_h);
		TheOutput.Init (0, "Packer Test");
		// Create the model
		_model.reset (new Model);
		// Create commander
		_commander.reset (new Commander (*_model,
										 _msgPrepro,// Message preprocessor
										 _h));		// Revisit: top app window
		// Create command vector
		_cmdVector.reset (new CmdVector (Cmd::Table, _commander.get ()));
		// Create the view manager
		_viewMan.reset (new ViewManager (_h, *_cmdVector));
		if (Initialize ())
		{
			success = true;
		}
	}
	catch (Win::Exception e)
	{
		// Revisit: this doesn't work
		// after displaying the dialog, we are called with OnFocus
		// even though we are still in OnCreate. OnFocus GP-faults
		TheOutput.Display (e);
		success = false;
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Initialization -- Unknown Error", Out::Error);
		success = false;
	}
	TheOutput.SetParent (_h);
	return true;
}

bool TestController::OnDestroy () throw ()
{
	Win::Quit ();
	return true;
}

bool TestController::OnFocus (Win::Dow::Handle winPrev) throw ()
{
	// Switch focus to view
	_viewMan->SetFocus ();
	return true;
}

bool TestController::OnSize (int width, int height, int flag) throw ()
{
	_viewMan->Size (width, height); 
	return true;
}

bool TestController::OnCommand (int cmdId, bool isAccel) throw ()
{
	MenuCommand (cmdId);
	return true;
}

bool TestController::OnMenuSelect (int id, Menu::State state, Menu::Handle menu) throw ()
{
	if (state.IsDismissed ())
	{
		// Menu dismissed
		_viewMan->SetStatusText ("Ready");
	}
	else if (!state.IsSeparator () && !state.IsSysMenu () && !state.IsPopup ())
	{
		// Menu item selected
		char const * cmdHelp = _cmdVector->GetHelp (id);
		_viewMan->SetStatusText (cmdHelp);
	}
	else
	{
		return false;
	}
	return true;
}

bool TestController::OnWheelMouse (int zDelta) throw ()
{
	return true;
}

bool TestController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	return false;
}

bool TestController::OnRegisteredMessage (Win::Message & msg) throw ()
{
	return false;
}

// Helpers

bool TestController::Initialize ()
{
	_viewMan->AttachMenu2Window (_h);
	return true;
}

void TestController::MenuCommand (int cmdId)
{
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
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal Error: Command execution failure", Out::Error);
	}
}

