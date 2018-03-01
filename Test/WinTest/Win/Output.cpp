//-----------------------------------------
// (c) Reliable Software 1998 -- 2003
//-----------------------------------------
#include "Output.h"
#include <Win/WinEx.h>
#include <string>
#include <sstream>

using namespace Out;

class SysMsg
{
public:
    SysMsg (DWORD errCode, HINSTANCE hModule = 0);
    ~SysMsg ();
    operator char const * () const  { return _msg; }
    char const * Text () const { return _msg; }
private:
    char * _msg;
};

std::string Sink::FormatExceptionMsg (Win::Exception const & ex)
{
	char const * exceptionMsg = ex.GetMessage ();
	assert (exceptionMsg != 0);

	std::ostringstream displayMsg;
	DWORD errCode = ex.GetError ();
	displayMsg << exceptionMsg;
	if (errCode != 0)
	{
		SysMsg sysMsg (errCode, ex.GetModuleHandle ());
		displayMsg << "\nSystem tells us: " << sysMsg.Text ();
	}
	char const * objectName = ex.GetObjectName ();
	if (objectName [0] != '\0')
	{
		displayMsg << "\n" << objectName;
	}
	return displayMsg.str ();
}

void Sink::DisplayException (Win::Exception const & ex, 
							 Win::Dow::Handle owner, 
							 char const * appName,
							 char const * title, 
							 int flags)
{
	try
	{
		if (ex.GetMessage () == 0)
			return;		// Quiet exception -- nothing to display

		std::string exceptionMsg = FormatExceptionMsg (ex);
		std::ostringstream displayMsg;
		displayMsg << appName << " problem: " << exceptionMsg.c_str ();
		::MessageBox (owner.ToNative (), displayMsg.str ().c_str (), title, flags);
	}
	catch (...)
	{
		Win::ClearError ();
	}
}

void Sink::Display (char const * msg, Severity sev, Win::Dow::Handle owner) const throw ()
{
	if (_verbose || sev == Error)
	{
		try
		{
			unsigned flag = MB_OK;
			if (_topWin.IsNull ())
				flag |= MB_TASKMODAL;
			if (_foreground)
				flag |= MB_SETFOREGROUND;
			flag |= sev;

			Win::Dow::Handle own = (owner.IsNull ()) ? _topWin : owner;
			if (::MessageBox (own.ToNative (), msg, _appName, flag) != 0)
				Win::ClearError ();	// If message box displayed successfuly clear any error code
									// set by system in the process of displaying message box.
		}
		catch (...)
		{
			Win::ClearError ();
		}
	}
}

void Sink::DisplayModal (char const * msg, Severity sev) const throw ()
{
	if (_verbose || sev == Error)
	{
		try
		{
			unsigned flag = MB_OK | MB_TASKMODAL;				
			if (_foreground)
				flag |= MB_SETFOREGROUND;
			flag |= sev;			
			if (::MessageBox (0, msg, _appName, flag) != 0)
				Win::ClearError ();	// If message box displayed successfuly clear any error code
									// set by system in the process of displaying message box.
		}
		catch (...)
		{
			Win::ClearError ();
		}
	}
}

void Sink::Display (Win::Exception const & ex) const throw ()
{
	unsigned flag = MB_ICONERROR | MB_OK;
	if (_topWin.IsNull ())
		flag |= MB_TASKMODAL;
	if (_foreground)
		flag |= MB_SETFOREGROUND;
	// don't release lock while displaying exception
	DisplayException (ex, _topWin, _appName, "Exception", flag);
}

Answer Sink::Prompt (char const * question, PromptStyle style, Win::Dow::Handle owner) const throw ()
{
	Win::Dow::Handle own = (owner.IsNull ()) ? _topWin : owner;
	unsigned flag = 0;
	if (_foreground)
		flag |= MB_SETFOREGROUND;
	flag |= style;
	int answer = -1;
	try
	{
		answer = ::MessageBox (own.ToNative (), question, _appName, flag);
		if (answer != 0)
			Win::ClearError ();	// If message box displayed successfuly clear any error code
								// set by system in the process of displaying message box.
	}
	catch (...)
	{
		Win::ClearError ();
	}

	switch (answer)
	{
	case IDYES:
		return Yes;
	case IDNO:
		return No;
	case IDRETRY:
		return Retry;
	case IDIGNORE:
		return Ignore;
	case IDABORT:
		return Abort;
	case IDOK:
		return OK;
	default:
		return Cancel;
	};
}

Answer Sink::PromptModal (char const * question, PromptStyle style) const throw ()
{
	unsigned flag = 0;
	if (_foreground)
		flag |= MB_SETFOREGROUND;
	flag |= style;
	flag |= MB_TASKMODAL;
	int answer = -1;
	try
	{
		answer = ::MessageBox (0, question, _appName, flag);
		if (answer != 0)
			Win::ClearError ();	// If message box displayed successfuly clear any error code
								// set by system in the process of displaying message box.
	}
	catch (...)
	{
		Win::ClearError ();
	}

	switch (answer)
	{
	case IDYES:
		return Yes;
	case IDNO:
		return No;
	case IDRETRY:
		return Retry;
	case IDIGNORE:
		return Ignore;
	case IDABORT:
		return Abort;
	case IDOK:
		return OK;
	default:
		return Cancel;
	};
}

unsigned PromptStyle::CalcDefButton () const throw ()
{
	switch (_buttons)
	{
	case RetryCancel:
		return Retry == _defaultAnswer ? MB_DEFBUTTON1 : MB_DEFBUTTON2;
	case YesNo :
		return Yes == _defaultAnswer ? MB_DEFBUTTON1 : MB_DEFBUTTON2;
	case YesNoCancel:
		if (Yes == _defaultAnswer)
			return MB_DEFBUTTON1;
		else if (No == _defaultAnswer)
			return MB_DEFBUTTON2;
		else
			return MB_DEFBUTTON3;
	case AbortRetryIgnore:
		if (Abort == _defaultAnswer)
			return MB_DEFBUTTON1;
		else if (Retry == _defaultAnswer)
			return MB_DEFBUTTON2;
		else
			return MB_DEFBUTTON3;
	default:
		return MB_DEFBUTTON1;
	};
}

SysMsg::SysMsg (DWORD errCode, HINSTANCE hModule)
    : _msg (0)
{
    if (errCode != 0)
    {
		unsigned flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;
		flags |= (hModule != 0)? FORMAT_MESSAGE_FROM_HMODULE: FORMAT_MESSAGE_FROM_SYSTEM;
        ::FormatMessage(
            flags,
            hModule,
            errCode,
            MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &_msg,
            0,
            NULL);
		if (_msg == 0 && hModule != 0)
		{
			// try system error
			::FormatMessage(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
				0,
				errCode,
				MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &_msg,
				0,
				NULL);
		}
		if (_msg == 0)
		{
			int len = 0;
			_msg = static_cast<char *> (::LocalAlloc (LPTR, 64));
			strcpy (_msg, "Error ");
			len = strlen (_msg);
			_ultoa (errCode, _msg + len, 10);
			strcat (_msg, " (0x");
			len = strlen (_msg);
			_ultoa (errCode, _msg + len, 16);
			strcat (_msg, ")");
		}
    }
}

SysMsg::~SysMsg ()
{
    if (_msg != 0)
        ::LocalFree (_msg);
}

