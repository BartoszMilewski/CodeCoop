//---------------------------
// (c) Reliable Software 1998
//---------------------------
#include <WinLibBase.h>
#include "Accelerator.h"
using namespace Accel;

template<>
void Win::Disposal<Accel::Handle>::Dispose (Accel::Handle h) throw ()
{
	::DestroyAcceleratorTable (h.ToNative ());
}

Maker::Maker (Accel::Item const * templ, Cmd::Vector & cmdVector)
{
    // Build accelerator definition table for Windows
    for (int i = 0; templ [i].key != 0; i++)
    {
        ACCEL accel;
        accel.fVirt = templ [i].flags;
        accel.key   = templ [i].key;
        accel.cmd   = cmdVector.Cmd2Id (templ [i].cmdName);
        _accelTable.push_back (accel);
    }
}

Accel::AutoHandle Maker::Create ()
{
	if (_accelTable.size () == 0)
		return Accel::AutoHandle ();
    Accel::AutoHandle h (::CreateAcceleratorTable (&_accelTable [0], _accelTable.size ()));
    if (h.IsNull ())
        throw Win::Exception ("Internal error: Cannot create accelerator table");
	return h;
}


