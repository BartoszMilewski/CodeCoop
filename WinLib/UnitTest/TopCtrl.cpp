//-----------------------------
//  (c) Reliable Software, 2005
//-----------------------------
#include "precompiled.h"
#include "TopCtrl.h"
#include "OutSink.h"
#include "Watch.h"
#include <Ctrl/Messages.h>
#include <File/FolderWatcher.h>

void RunTest (std::ostream & out);

TopCtrl::TopCtrl ()
	: _initMsg ("InitMessage"),
	_ready (false),
	_timer (1)
{}

TopCtrl::~TopCtrl ()
{}

bool TopCtrl::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	Win::Dow::Handle win = GetWindow ();
	try
	{
		win.SetText ("Unit Test");
		Win::EditMaker editMaker (_h, 1);
		editMaker.Style () 
			<< Win::Edit::Style::MultiLine 
			<< Win::Edit::Style::AutoVScroll 
			<< Win::Style::Ex::ClientEdge
			<< Win::Style::AddVScrollBar;
		_output.Reset (editMaker.Create ());
		_watch.reset (new Watch (win));
		_timer.Attach (win);

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
	std::ostream winStream (&_output);
	_timer.Set (100);
	RunTest (winStream);
	winStream << std::endl; // flush
}

bool TopCtrl::OnDestroy () throw ()
{
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
	return false;
}

bool TopCtrl::OnUserMessage (Win::UserMessage & msg) throw ()
{
	if (msg.GetMsg () == UM_FOLDER_CHANGE)
	{
		FolderChange (reinterpret_cast<FWatcher *>(msg.GetLParam ()));
		return true;
	}
	return false;
}

bool TopCtrl::OnTimer (int id) throw ()
{
	static bool first = true;
	if (first)
	{
		_watch->RemoveFolder ();
		first = false;
	}
	else
	{
		_watch->AddFolder ();
		first = true;
	}
	return true;
}


bool TopCtrl::OnSize (int width, int height, int flag) throw ()
{
	if (_ready)
		_output.Move (0, 0, width, height);
	return true;
}

void TopCtrl::FolderChange (FWatcher * watcher)
{
	std::string change;
	std::ostream out (&_output);
	while (watcher->RetrieveChange (change))
	{
		out << "Change in: " << change << std::endl;
	}
}
