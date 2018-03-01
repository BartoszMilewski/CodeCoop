//---------------------------
// (c) Reliable Software 2007
//---------------------------
#include "precompiled.h"
#include "GlobalLock.h"

// Data shared between different mappings of the SccDll
#pragma comment (linker, "/SECTION:.shared,RWS")

#pragma data_seg (".shared")

volatile long TheSharedLong = 0;

#pragma data_seg ()

Win::AtomicCounterPtr TheGlobalLock (&TheSharedLong);