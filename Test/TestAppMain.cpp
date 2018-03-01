//-------------------------------------------------
// TestAppMain.cpp
// (c) Reliable Software 2003
//-------------------------------------------------

#include "precompiled.h"
#include <Win/WinMain.h>
#include <Ex/WinEx.h>
#include <File/ErrorLog.h>
#include <Ctrl/Output.h>
#include <Dbg/Memory.h>

#include <Handlers.h>

#include "TestAppUtil.h"

//	Tests
namespace Test_XString
{
	void RunAll ();
};

namespace Test_XLong
{
	void RunAll ();
}

namespace Test_Active
{
	void RunAll ();
}

namespace Test_License
{
	void RunAll ();
}

//	Misc declarations

Out::Sink TheOutput;

void UnexpectedHandler ();

//#if defined (_DEBUG)
//#include "MemCheck.h"
//static HeapCheck heapCheck;
//#endif

int Win::Main (Win::Instance hInst, char const * cmdParam, int cmdShow)
{
	UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

	int status = 0;
	try
	{
		//	Make sure our infrastructure is working
		{
			try
			{
				{
					Dbg::NoMemoryAllocations noMemAllocs;
					noMemAllocs;	//	prevent warning C4101 by referencing variable

					//	should throw
					std::auto_ptr<int> p (new int);
				}

//				Assert (0 && "Dbg::NoMemoryAllocations not working");	//	to be enabled...
			}
			catch (...)
			{
			}
		}

		//	On to the tests
		Test_XLong::RunAll ();
		Test_XString::RunAll ();
		Test_Active::RunAll ();
		Test_License::RunAll ();
	}
	catch (Win::Exception e)
	{
		TheOutput.Display (e);
	}
	catch (char const * msg)
	{
		TheOutput.Display (msg, Out::Error);
	}
	catch (...)
	{
		Win::ClearError ();
		TheOutput.Display ("Internal error: Unknown exception", Out::Error);
	}
	return status;
}

void UnexpectedHandler ()
{
	TheErrorLog << "Unexpected system exception occurred during program execution." << std::endl;
	TheErrorLog << "The program will now exit." << std::endl;
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
