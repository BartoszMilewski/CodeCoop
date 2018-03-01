#if !defined (DBG_ALL_H)
#define DBG_ALL_H
//------------------------------------
// (c) Reliable Software 2001
//------------------------------------

//	Use Assert(expr) for assertions
#include "Assert.h"

//	declare object Dbg::LeakTrack leaktrack globally for leak tracking
#include "Memory.h"

//	To log output to the debug stream just use the 'dbg' as an ostream:
//		E.g.:  dbg << "Just a simple message " << n << std::endl;
#include "Out.h"

#endif