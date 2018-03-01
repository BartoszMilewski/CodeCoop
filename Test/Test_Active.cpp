#include "precompiled.h"

#include <Sys/Active.h>
#include <Dbg/Out.h>
#include <Dbg/Memory.h>

namespace Test_Active
{

class TestActiveObject : public ActiveObject
{
public:
	TestActiveObject (int idx)
		: _idx (idx)
	{
		dbg << "TestActiveObject " << _idx << " constructor" << std::endl;
	}

	~TestActiveObject ()
	{
		dbg << "TestActiveObject " << _idx << " destructor" << std::endl;
	}

	//	worker thread
	void Run ()
	{
		for (;;)
		{
			std::auto_ptr<int> p(new int);

			Dbg::NoMemoryAllocations noAllocs;
			noAllocs;	//	prevent warning C4101

			p.reset ();

			if (IsDying ())
				return;
		}
	}

	//	master thread
	void FlushThread ()
	{
	}
private:
	int _idx;
};

void RunAll ()
{
	{
		dbg << "About to create" << std::endl;
		auto_active<TestActiveObject> testObject (new TestActiveObject (1));
		auto_active<TestActiveObject> testObject2 (new TestActiveObject (2));
		auto_active<TestActiveObject> testObject3 (new TestActiveObject (3));

		Assert (!testObject.empty ());
		testObject.SetWaitForDeath ();
		testObject2.SetWaitForDeath ();
		testObject3.SetWaitForDeath ();

		Sleep (1000);

		dbg << "About to reset" << std::endl;
		testObject.reset ();
		dbg << "Done resetting" << std::endl;
	}

	dbg << "Out of scope" << std::endl;

	{
//		TestActiveObject objectOnStack;
	}
}

}