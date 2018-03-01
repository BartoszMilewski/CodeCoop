#if !defined (GLOBALLOCK_H)
#define GLOBALLOCK_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------
#include "precompiled.h"
#include <Sys/Synchro.h>

// Locks all instances of SCC not to execute any commands
extern Win::AtomicCounterPtr TheGlobalLock;

#endif
