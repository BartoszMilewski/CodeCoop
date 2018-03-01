//---------------------------
//    Main.cpp
// (c) Reliable Software 2000
//---------------------------

#include "HeaderDetails.h"

#include "OutputSink.h"

#include <Ex/WinEx.h>
#include <Handlers.h>


#include <commctrl.h>
#include <new.h>
#include <exception>

void NewHandler ();
void UnexpectedHandler ();

extern bool CollectData (HINSTANCE hInst, HeaderDetails & details);
extern void GenerateScript (HeaderDetails const & details);

int Main (HINSTANCE hInst, LPSTR cmdParam, int cmdShow)
{
	NewHandlerSwitch newHandler (NewHandler);
	UnexpectedHandlerSwitch unexpHandler (UnexpectedHandler);

    try
    {
		HeaderDetails details;
		if (CollectData (hInst, details))
		{
			GenerateScript (details);
		}
	}
    catch (Win::Exception e)
    {
        TheOutput.Display (e);
    }
    catch (...)
    {
		Win::ClearError ();
        TheOutput.Display ("Internal error.", Out::Error);
	}
	
	return 0;
}

void NewHandler ()
{
    throw Win::Exception ("Internal error: Out of memory");
}

void UnexpectedHandler ()
{
	TheOutput.Display ("Unexpected system exception occurred during program execution.\n"
					   "The program will now exit.");
	terminate ();
}
