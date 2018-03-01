#include "precompiled.h"
#include "XLong.h"
#include <Dbg/Memory.h>

namespace Test_XLong
{

void Test_Basics ()
{
	{
		XLong xLong;
		Assert (xLong.Get () == xLong.GetDefaultValue ());
		Assert (xLong.Get () == 0);
	}

	{
		XLongWithDefault<-1> xLong (1);
		Assert (xLong.Get () != xLong.GetDefaultValue ());
		Assert (xLong.Get () == 1);

		xLong.Clear ();
		Assert (xLong.Get () == xLong.GetDefaultValue ());
		Assert (xLong.Get () == -1);
	}
}
void Test_Commit ()
{
	XLong xLong (7);

	xLong.BeginTransaction ();
	Assert (xLong.XGet () == 7);

	xLong.XSet (22);
	Assert (xLong.XGetOriginal () == 7);
	Assert (xLong.XGet () == 22);

	{
		//	Demonstrated Bug #754
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	to avoid build warning
		xLong.CommitTransaction ();
	}

	Assert (xLong.Get () == 22);

	xLong.Clear ();
	Assert (xLong.Get () == 0);
}

void Test_Abort ()
{
	XLongWithDefault<22>  xLong (7);

	Assert (xLong.Get () != xLong.GetDefaultValue ());

	xLong.BeginClearTransaction ();
	Assert (xLong.XGet () == 22);
	Assert (xLong.XGetOriginal () == 7);

	{
		//	Demonstrated Bug #754
		Dbg::NoMemoryAllocations noMemAllocs;
		noMemAllocs;	//	to avoid build warning
		xLong.AbortTransaction ();
	}

	Assert (xLong.Get () == 7);
}

void RunAll ()
{
	//	Serializable interface (during xaction) (to be done)
	//	Serialize
	//	Deserialize

	//	Transactable interface
	Test_Basics ();
	Test_Commit ();
	Test_Abort ();
}

}