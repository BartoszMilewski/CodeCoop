#include "Integrator.h"
#include <vcl.h>
#include <toolsapi.hpp>

#pragma hdrstop
USEFORM("CheckInDlg.cpp", CheckInDlg);
//---------------------------------------------------------------------------
#pragma package(smart_init)

// Register the wizard in a package.

namespace ToolsAPI
{
   extern PACKAGE void __fastcall RegisterPackageWizard(const _di_IOTAWizard Wizard);
}

namespace Borland6
{
    void PACKAGE __fastcall Register ()
    {
        RegisterPackageWizard (new Integrator);
    }
}

int WINAPI DllEntryPoint (HINSTANCE hinst, unsigned long reason, void*)
{
        return 1;
}

