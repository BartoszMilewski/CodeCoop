#if !defined(DBG_OUT_H)
#define DBG_OUT_H
//------------------------------------
// (c) Reliable Software 2001 -- 2002
//------------------------------------

//
//	Debug output support
//
//	To send output to the debug stream just use the 'dbg' as an ostream:
//		E.g.:  dbg << "Just a simple message " << n << std::endl;
//  Warning: don't use before or after main()
//  When debugging multithreaded application always end debug output with std::endl to avoid deadlocking 

#include <iosfwd>

#if defined(DIAGNOSTIC)

namespace Dbg
{
	extern std::ostream TheOutStream;
	
	class Lock
	{
		// this class serves only one purpose: ostream operator << is defined for it
		// which let's us manipulate dbg output
	public:
		Lock (bool isLogging, char const * component)
			: _isLogging (isLogging),
			  _component (component)
		{}
	public:
		char const * _component;
		bool		 _isLogging;
	};

	std::ostream& operator << (std::ostream& os, Dbg::Lock & lock);
}

#define dbg Dbg::TheOutStream << Dbg::Lock (DBG_LOGGING, COMPONENT_NAME)

// DBG_LOGGING: true/false
// if you want to turn off debug logging in a specific file,
// add these lines after #include "precompiled.h":
// #undef DBG_LOGGING
// #define DBG_LOGGING false
// Note: do not change the definition of this macro inside header files,
// because this also changes logging settings in files that include the headers.
// Change the value of the macro only in source files!

#else

//	In non-debug builds we want any calls to the debug stream to disappear.

//	Make sure that in non-debug builds we completely get rid of calls to dbg,
//	even if we're calling functions and trying to send the results.  E.g.,
//
//		dbg << pFoo->SomeFunction(...)
//
//	should disappear completely.  It's not enough to have the dbg stream do
//	nothing.  We make sure the call to the stream is on the right of a
//	logical and so that it never gets evaluated.  And since it's never used,
//	we don't even need a real object.  Here we construct an ostream out of
//	nothing (literally).
//

#define dbg		0 && (*((std::ostream *) 0))

#endif

#endif
