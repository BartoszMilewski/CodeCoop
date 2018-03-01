#include "precompiled.h"
#include "XString.h"
#include <Dbg/Memory.h>

namespace Test_XString
{

//	VS.Net allocates memory for strings longer than 16 characters
char const *longString = "This string is long enough to cause a memory allocation";

void Test_Commit ()
{
	XString xString;
	Assert (xString.IsEmpty ());

	xString.BeginTransaction ();
	Assert (xString.XIsEmpty ());

	xString.XSet (longString);
	Assert (!xString.XIsEmpty ());

	{
		//	Demonstrated Bug #754
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	to avoid build warning
		xString.CommitTransaction ();
	}

	Assert (strcmp (xString.c_str (), longString) == 0);
}

void Test_Abort ()
{
	char const *originalData = "OriginalData";
	XString xString (originalData);
	Assert (!xString.IsEmpty ());
	Assert (strcmp (xString.c_str (), originalData) == 0);

	xString.BeginTransaction ();
	Assert (!xString.XIsEmpty ());

	xString.XSet (longString);
	Assert (!xString.XIsEmpty ());
	Assert (strcmp (xString.XGetString ().c_str (), longString) == 0);
	xString.XClear ();
	Assert (xString.XIsEmpty ());
	xString.XSet (longString);
	Assert (strcmp (xString.XGetString ().c_str (), longString) == 0);

	{
		//	Demonstrated Bug #754
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	to avoid build warning
		xString.AbortTransaction ();
	}

	Assert (strcmp (xString.c_str (), originalData) == 0);
}

void RunAll ()
{
	//	Serializable interface (during xaction) (to be done)
	//	Serialize
	//	Deserialize

	//	Transactable interface
	Test_Commit ();
	Test_Abort ();
}

}
