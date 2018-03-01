// (c) Reliable Software 2003
#include "precompiled.h"
#include "TestController.h"
#include <ctime>
#include <cstdlib>

Out::Sink TheOutput;

bool TestController::OnTimer (int id) throw ()
{
	_timer.Kill ();
	RunTest (*this);
	return true;
}

bool TestController::OnCreate (Win::CreateData const * create, bool & success) throw ()
{
	TheOutput.SetParent (_h);
	Win::EditMaker editMaker (_h, 1);
	editMaker.Style () << Win::Edit::Style::MultiLine 
		<< Win::Edit::Style::AutoVScroll 
		<< Win::Style::Ex::ClientEdge
		<< Win::Style::AddVScrollBar;
	_edit.Reset (editMaker.Create ());
	_timer.Attach (_h);
	_timer.Set (100);
	std::srand (static_cast<unsigned int>(time (0)));
	success = true;
	return true;
}

void TestController::PutLine (char const * text)
{
	_edit.Append (text);
	_edit.Append ("\r\n");
}

void TestController::PutBoldLine (char const * text)
{
	_edit.Append ("    [");
	_edit.Append (text);
	_edit.Append ("]\r\n");
}

void TestController::PutMultiLine (std::string const & text)
{
	unsigned beg = 0;
	unsigned end = text.find ('\n', beg);
	while (end != std::string::npos)
	{
		_edit.Append (text.substr (beg, end - beg).c_str ());
		_edit.Append ("\n");
		beg = end + 1;
		end = text.find ('\n', beg);
	}
	_edit.Append (text.substr (beg).c_str ());
}

