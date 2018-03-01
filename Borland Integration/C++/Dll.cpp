#include "Integrator.h"

#include <vcl.h>
#include <windows.h>
#include <ToolsApi.hpp>

#pragma hdrstop
//---------------------------------------------------------------------------
//   Important note about DLL memory management when your DLL uses the
//   static version of the RunTime Library:
//
//   If your DLL exports any functions that pass String objects (or structs/
//   classes containing nested Strings) as parameter or function results,
//   you will need to add the library MEMMGR.LIB to both the DLL project and
//   any other projects that use the DLL.  You will also need to use MEMMGR.LIB
//   if any other projects which use the DLL will be performing new or delete
//   operations on any non-TObject-derived classes which are exported from the
//   DLL. Adding MEMMGR.LIB to your project will change the DLL and its calling
//   EXE's to use the BORLNDMM.DLL as their memory manager.  In these cases,
//   the file BORLNDMM.DLL should be deployed along with your DLL.
//
//   To avoid using BORLNDMM.DLL, pass string information using "char *" or
//   ShortString parameters.
//
//   If your DLL uses the dynamic version of the RTL, you do not need to
//   explicitly add MEMMGR.LIB as this will be done implicitly for you
//---------------------------------------------------------------------------

extern "C" __export bool __stdcall INITWIZARD0001 (
        IBorlandIDEServices* services,
        TWizardRegisterProc registerProc,
        TWizardTerminateProc& terminate)
{
   BorlandIDEServices = services; // only needed when linking without packages

   // Save the application window handle.
   _di_IOTAServices ota_services;
   services->QueryInterface (IID_IOTAServices, (void**) &ota_services);
   Application->Handle = ota_services->GetParentHandle ();

   // Create and register the wizard(s).
   registerProc (new Integrator ());

   // Return true for success, and false for failure.
   return true;
}

#pragma argsused
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved)
{
        return 1;
}

