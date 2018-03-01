// (c) Reliable Software 2003
#include "TestController.h"

Out::Sink TheOutput;

TestController::TestController ()
: _timer (0), 
  _lineReadyMsg (1)
{}

bool TestController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	TheOutput.Init (_h, "Test");
	// Create multi-line edit control
	Win::EditMaker editMaker (_h, 1);
	editMaker.Style () << Win::Edit::Style::MultiLine 
		<< Win::Edit::Style::AutoVScroll 
		<< Win::Style::AddClientEdge
		<< Win::Style::AddVScrollBar;
	_edit.Reset (editMaker.Create ());

	// Set the timer to start testing. We want to finish processing OnCreate.
	_timer.Attach (_h);
	_timer.Set (100);
	success = true;
	return true;
}

bool TestController::OnTimer (int id) throw ()
{
	try
	{
		if (_timer == id)
		{
			_timer.Kill ();
			// The tests start here
			_tester = StartTest (*this);
		}
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (...)
	{
		TheOutput.Display ("Unexpected error", Out::Error);
	}
	return true;
}

bool TestController::OnUserMessage (Win::UserMessage & msg) throw ()
{
	if (msg == _lineReadyMsg)
	{
		try
		{
			// Some lines were added to the queue. Display them.
			Win::Lock lock (_critSect);
			while (_lineQueue.size () != 0)
			{
				std::string text (_lineQueue.front ());
				_edit.Append (text.c_str ());
				_edit.Append ("\r\n");
				_lineQueue.pop_front ();
			}
		}
		catch (Win::Exception e)
		{
			TheOutput.Display (e);
		}
		catch (...)
		{
			TheOutput.Display ("Unexpected error", Out::Error);
		}
		return true;
	}
	return false;
}

// WinOut interface implementation

void TestController::PutLine (char const * text)
{
	_edit.Append (text);
	_edit.Append ("\r\n");
}

// Queue a line of text and post a user-defined message
// Note: this method may be called from another thread!
void TestController::PostLine (char const * text)
{
	Win::Lock lock (_critSect);
	_lineQueue.push_back (text);
	GetWindow ().PostMsg (_lineReadyMsg);
}
