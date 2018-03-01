//---------------------------------------------------------------------------
#include "Integrator.h"
#include <vcl.h>
#pragma hdrstop
USEPACKAGE("vcl50.bpi");
USEUNIT("Integrator.cpp");
USELIB("WinLib.lib");
USEUNIT("..\..\..\co-op31\Common\SccDllFile.cpp");
USEUNIT("..\..\..\co-op31\Common\PathRegistry.cpp");
USEUNIT("..\..\..\co-op31\CmdLine\Common\SccProxy.cpp");
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

//   Package source.
//---------------------------------------------------------------------------

namespace ToolsAPI
{
   extern PACKAGE void __fastcall RegisterPackageWizard(const _di_IOTAWizard Wizard);
}

namespace Borland5
{
    void PACKAGE __fastcall Register ()
    {
        RegisterPackageWizard (new Integrator);
    }
}

#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void*)
{
        return 1;
}
//---------------------------------------------------------------------------
