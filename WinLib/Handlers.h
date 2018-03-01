#if !defined (HANDLERS_H)
#define HANDLERS_H
//---------------------------
// (c) Reliable Software 2000
//---------------------------

#include <Ex\Winex.h>

#include <new.h>
#include <exception>

int MsNewHandler (size_t);

typedef void (*HandlerPtr) ();
typedef void (*StructuredExceptionTranslatorPtr)(unsigned int, PEXCEPTION_POINTERS);

extern HandlerPtr GlobalNewHandler;

class NewHandlerSwitch
{
public:
	NewHandlerSwitch (void (*pFun) ())
	{
		GlobalNewHandler = pFun;
		_oldHandler = _set_new_handler (MsNewHandler);
	}
	~NewHandlerSwitch ()
	{
		_set_new_handler (_oldHandler);
	}
private:
	int (*_oldHandler) (size_t);
};

class UnexpectedHandlerSwitch
{
public:
	UnexpectedHandlerSwitch (void (*pFun) ())
	{
		_oldHandler = set_unexpected (pFun);
	}
	~UnexpectedHandlerSwitch ()
	{
		set_unexpected (_oldHandler);
	}
private:
	HandlerPtr _oldHandler;
};

// Structured Exception Translator -- converts Windows Structured Exceptions into C++ Exceptions
class SET
{
public:
	// Call this function for each thread that needs exception translation
	static void EnableExceptionTranslation ()
	{
		_set_se_translator (&ExceptionTranslator);
	}

private:
	static void _cdecl ExceptionTranslator (unsigned int exCode, PEXCEPTION_POINTERS ep)
		throw (Win::ExitException);
};

#endif
