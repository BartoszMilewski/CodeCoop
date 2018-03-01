// (c) Reliable Software 2003
#include "Tester.h"
#include "WinOut.h"
#include <Win/Active.h>
#include <sstream>
#include <cstdlib>

// Creates a worker thread and runs it
class MyActiveTest: public ActiveObject
{
public:
	MyActiveTest (WinOut & out)
		: _out (out)
	{}
	void Run ();
	void FlushThread () {}
private:
	WinOut & _out;
};

// Tester object, may run single-threaded tests in its constructor
// Or start a separate active-test thread
class MyTester: public Tester
{
public:
	MyTester (WinOut & out)
		: _out (out)
	{
		_out.PutLine ("Begin single-threaded test");
		for (int i = 0; i < 8; ++i)
		{
			Win::Sleep (100);
			_out.PutLine ("testing...");
			// Put your test code here. Use _out.PutLine for display
		}
		_out.PutLine ("End single-threaded test\r\n");

		_out.PutLine ("Multi-threaded test");
		_test.reset (new MyActiveTest (_out));
	}
private:
	WinOut	& _out;
	auto_active<MyActiveTest> _test;
};

// Implementation of the factory function
std::auto_ptr<Tester> StartTest (WinOut & out)
{
	return std::auto_ptr<Tester> (new MyTester (out));
}

// This method is run by a worker thread of the ActiveObject.
// Must use _out.PostLine for text output.
void MyActiveTest::Run ()
{
	_out.PostLine ("Hello from worker thread");
	for (int i = 0; i < 8; ++i)
	{
		if (IsDying ())
			return;
		Win::Sleep (100);
		_out.PostLine ("testing...");
	}
	_out.PostLine ("Goodbye from worker thread");
}

