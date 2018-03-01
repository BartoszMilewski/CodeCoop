#if !defined(DBG_ASSERT_H)
#define DBG_ASSERT_H
//------------------------------------
// (c) Reliable Software 2001
//------------------------------------

//	Assertion support
//
//	The Windows _ASSERTE has exactly the behavior we want for assertions,
//	so we use it.  Note that we set a report hook in Dbg/Out.cpp so
//	that we can see assertions along with any other debug output.

#define Assert(expr)	_ASSERTE(expr)

#if defined(_DEBUG)

//	only for use within Assert
namespace Dbg
{
	bool IsMainThread ();
	bool OutputFutureAssert (char const *file, int line, char const *expr);
}

#define FutureAssert(expr)	(void) ((expr) || (Dbg::OutputFutureAssert (__FILE__, __LINE__, #expr)))

class boolDbg
{
public:
	boolDbg () : _value (false) {}
	void Set () { _value = true; }
	void Clear () { _value = false; }

	operator bool () const { return _value; }

private:
	bool	_value;
};

#else

#define FutureAssert(expr) ((void)0)

#endif

//	Don't let asserts (with a lowercase a) creep into the code

#undef assert

#endif
