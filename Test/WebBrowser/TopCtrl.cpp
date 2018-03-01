//-------------------------------
// (c) Reliable Software, 2004-06
//-------------------------------
#include "precompiled.h"
#include "TopCtrl.h"
#include "OutSink.h"
#include "WebBrowserRegistry.h"
#include "Resource/resource.h"
#include "InPlaceBrowser.h"
#include <Sys/WinString.h>
#include <Win/MsgLoop.h>
#include <File/Path.h>


void EventSink::BeforeNavigate (std::string const & url, Automation::Bool & cancel)
{
	if (url == "C:\\wiki\\foo")
	{
		CurrentFolder path;
		_browser->Navigate (path.GetFilePath ("foo.html"));
		cancel.Set (true);
	}
}


TopCtrl::TopCtrl ()
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
	return true;
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

bool TopCtrl::OnSize (int width, int height, int flag) throw ()
{
	try
	{
		if (_browser.get ())
			_browser->Size (width, height);
		else if (width != 0 && height != 0)
		{
			_browser.reset (new InPlaceBrowser (GetWindow ()));
			_browser->SetEventHandler (&_eventSink);
			_eventSink.Attach (_browser.get ());

			CurrentFolder path;
			_browser->Navigate (path.GetFilePath ("index.htm"));
		}
	}
	catch (Win::Exception ex)
	{
		TheOutput.Display (ex);
	}
	return true;
}