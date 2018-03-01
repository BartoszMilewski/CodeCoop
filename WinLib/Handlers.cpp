//----------------------------------
// (c) Reliable Software 2000 - 2005
//----------------------------------

#include <WinLibBase.h>
#include "Handlers.h"

#include <Delayimp.h>   // For delay dll load error handling
#include <StringOp.h>

#include <iomanip>

HandlerPtr GlobalNewHandler = 0;

int MsNewHandler (size_t)
{
	GlobalNewHandler ();
	return 0;
}

void _cdecl SET::ExceptionTranslator (unsigned int exCode, PEXCEPTION_POINTERS ep) throw (Win::ExitException)
{
	// For all structured exceptions we throw Win::ExitException
	if (exCode == VcppException (ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND) ||
		exCode == VcppException (ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))
	{
		if (ep != 0)
		{
			// If this is a Delay-load problem, ExceptionInformation[0] points 
			// to a DelayLoadInfo structure that has detailed error info
			PDelayLoadInfo pdli = reinterpret_cast<PDelayLoadInfo>(ep->ExceptionRecord->ExceptionInformation [0]);
			std::string info ("The required dynamic link library ");
			info += pdli->szDll;
			if (exCode == VcppException (ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND))
			{
				// The DLL module was not found at runtime
				info += " was not found";
			}
			else
			{
				// The DLL module was found but it doesn't contain the function
				info += " does not contain function ";
				if (pdli->dlp.fImportByName)
				{
					info += pdli->dlp.szProcName;
				}
				else
				{
					info += "with ordinal ";
				    info += ToString (pdli->dlp.dwOrdinal);
				}
			}
			info += '.';
			throw Win::ExitException ("Fatal error: missing or un-useable component.", info.c_str ()); 
		}
		throw Win::ExitException ("Fatal error: missing or un-useable Dynamic Link Library.");
	}
	else if (exCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if (ep != 0)
		{
			PEXCEPTION_RECORD exRec = ep->ExceptionRecord;
			if (exRec != 0)
			{
				std::ostringstream info;
				info << "The instruction at address '0x";
				info << std::hex << std::setw (8) << std::setfill ('0') << exRec->ExceptionAddress;
				info << "' referenced memory at address '0x";
				info << std::hex << std::setw (8) << std::setfill ('0') << exRec->ExceptionInformation [1];
				info << "'. The memory could not be ";
				if (exRec->ExceptionInformation [0] == 0)
					info << "read.";
				else
					info << "written.";
				throw Win::ExitException ("Fatal error: access violation.", info.str ().c_str ());
			}
		}
		throw Win::ExitException ("Fatal error: access violation.");
	}
	// For all other structured exception codes throw this one ExitException
	throw Win::ExitException ("Fatal error: unrecoverable error occurred during program execution.");
}
