//----------------------------------
// (c) Reliable Software 2004 - 2005
//----------------------------------

#include "precompiled.h"
#include "OutputSink.h"

#if !defined (NDEBUG)

namespace UnitTest
{
	void Run () throw ();

	// * Unit test prototypes:
	extern void BitStream ();
	extern void EmailDiag ();
	// Caller catches all exceptions
}

void UnitTest::Run () throw ()
{
	try
	{
		// * Unit test calls:
		UnitTest::BitStream ();
	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException (e, 0, "Code Co-op Dispatcher", "Unit test failed");
	}
	catch (std::bad_alloc)
	{
		TheOutput.Display ("Unit test failed: Out of memory");
	}
	catch ( ... )
	{
		TheOutput.Display ("Unknown unit test failure");
	}
}
 
#endif