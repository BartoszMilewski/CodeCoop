//----------------------------------
// (c) Reliable Software 2004 - 2005
//----------------------------------

#include "precompiled.h"
#include "OutputSink.h"

#if !defined (NDEBUG) && !defined (BETA)

// * Unit test prototypes:
void MemberDescriptionTest ();
void ClusterOwnerTest ();
void TopoSortTest();

// Caller catches all exceptions
void UnitTest () throw ()
{
	try
	{
		// * Unit test calls:
		TopoSortTest();
		MemberDescriptionTest ();
		ClusterOwnerTest ();
	}
	catch (Win::Exception e)
	{
		TheOutput.DisplayException (e, 0, "Code Co-op", "Unit test failed");
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