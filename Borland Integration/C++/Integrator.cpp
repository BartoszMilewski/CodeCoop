// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------
#include "Integrator.h"
#include "CheckInDlg.h"
#include "SccProxy.h"
#include <StringOp.h>
#include <Ex/WinEx.h>
#include <File/File.h>
#include <Sys/WinString.h>
#include <LightString.h>
#include <iostream>
#include <assert.h>

char Integrator::CodeCoopExpert [] = "Reliable Software.Code Co-op Expert";

// IUnknown
HRESULT STDMETHODCALLTYPE Integrator::QueryInterface (
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID (riid, __uuidof (IUnknown)))
        *ppvObject = static_cast<IUnknown *> (this);
    else if (IsEqualIID (riid, __uuidof (IOTANotifier)))
        *ppvObject = static_cast<IOTANotifier *> (this);
    else if (IsEqualIID (riid, __uuidof (IOTAWizard)))
        *ppvObject = static_cast<IOTAWizard *> (this);

    if (*ppvObject != NULL)
    {
        reinterpret_cast<IUnknown *>(*ppvObject)->AddRef ();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE Integrator::AddRef ()
{
    return ::InterlockedIncrement (&_refCount);
}

ULONG STDMETHODCALLTYPE Integrator::Release ()
{
  if (::InterlockedDecrement (&_refCount) == 0)
  {
    delete this;
    return 0;
  }
  return _refCount;
}

// IOTAWizard
void __fastcall Integrator::Execute ()
{
    assert (!"never called");
}

AnsiString __fastcall Integrator::GetIDString ()
{
    return CodeCoopExpert;
}

AnsiString __fastcall Integrator::GetName ()
{
    return CodeCoopExpert;
}

TWizardState __fastcall Integrator::GetState ()
{
    return TWizardState() << wsEnabled;
}

Integrator::Integrator ()
: _refCount (0)
{
    // Create Coop menu
    _coopExe.reset (NewItem ("Launch &Code Co-op", 0, false, true, OnLaunchCoop, 0, "CoopExe"));
    _checkOut.reset (NewItem ("Check-&Out", 0, false, true, OnCheckOut, 0, "CheckOut"));
    _uncheckOut.reset (NewItem ("&Uncheck-Out", 0, false, true, OnUncheckOut, 0, "UncheckOut"));
    _checkIn.reset (NewItem ("Check-&In...", 0, false, true, OnCheckIn, 0, "CheckIn"));
    _separator.reset (NewLine ());
    _about.reset (NewItem ("&About...", 0, false, true, OnAbout, 0, "About"));

    TMenuItem const * items [6];
    items [0] = _coopExe.get ();
    items [1] = _checkOut.get ();
    items [2] = _uncheckOut.get ();
    items [3] = _checkIn.get ();
    items [4] = _separator.get ();
    items [5] = _about.get ();

    _coopMenu.reset (NewSubMenu ("C&ode Co-op", 0, "CodeCoop", items, 5, true));  // 6 - doesn't work
    _coopMenu->OnClick = OnClickCoopMenu;

    // Install the menu into main IDE menu
    _di_INTAServices ntaServices;
    // Revisit: Handle exceptions
    BorlandIDEServices->QueryInterface (
                                __uuidof (INTAServices),
                                (void **)&ntaServices);

    TMainMenu * mainMenu = ntaServices->GetMainMenu ();
    mainMenu->Items->Insert (mainMenu->Items->Count - 2, _coopMenu.get ());
}

Integrator::~Integrator ()
{
    _di_INTAServices ntaServices;
    if (SUCCEEDED (BorlandIDEServices->QueryInterface (IID_INTAServices, (void**)&ntaServices)))
    {
        TMainMenu * mainMenu = ntaServices->GetMainMenu ();
        mainMenu->Items->Remove (_coopMenu.get ());
    }
}

void __fastcall Integrator::OnClickCoopMenu (TObject *Sender)
{
    try
    {
        std::vector<std::string> files;
        GetCurrentModuleFiles (files);

        if (files.size () == 0)
        {
            _checkIn->Enabled = false;
            _uncheckOut->Enabled = false;
            _checkOut->Enabled = false;
        }
        else
        {
            // string weirdness
            std::vector<const char*> tmp (files.size ());
            for (int i = 0; i < files.size (); ++i)
               tmp [i] = files [i].c_str ();

            std::vector<long> statuses (files.size ());
            SccProxy sccProxy;
            if (!sccProxy.Status (files.size (), &tmp [0], &statuses [0]))
              throw Win::Exception ("Cannot query statuses of the current module files.", tmp [0]);

            bool isAnyCheckedIn = false, isAnyNotCheckedIn = false, isAnyCheckedOut = false;
            for (int i = 0; i < statuses.size (); ++i)
            {
                if ((statuses [i] & 1) == 0) // not controlled
                {
                    isAnyNotCheckedIn = true;
                }
                else if ((statuses [i] & 2) == 0) // not checked out
                {
                    isAnyCheckedIn = true;
                }
                else // checked out
                {
                    isAnyCheckedOut = true;
                    isAnyNotCheckedIn = true;
                }
            }
            _checkIn->Enabled = isAnyNotCheckedIn;
            _checkOut->Enabled = isAnyCheckedIn;
            _uncheckOut->Enabled = isAnyCheckedOut;
        }
    }
    catch (Win::Exception e)
    {
        _checkIn->Enabled = false;
        _uncheckOut->Enabled = false;
        _checkOut->Enabled = false;
        DisplayEx ("On Menu Click", e);
    }
    catch (...)
    {
        _checkIn->Enabled = false;
        _uncheckOut->Enabled = false;
        _checkOut->Enabled = false;

    	Win::ClearError ();
        ::MessageBox (0, "On Menu Click: Unknown problem", "Code Co-op", MB_OK);
    }
}

void __fastcall Integrator::OnLaunchCoop (TObject * Sender)
{
    try
    {
    	SccProxy sccProxy;
        if (!sccProxy.ExecuteCoop ())
	    throw Win::Exception ("Cannot execute Code Co-op.");
    }
    catch (Win::Exception e)
    {
        DisplayEx ("Launch Code Co-op", e);
    }
    catch (...)
    {
    	Win::ClearError ();
        ::MessageBox (0, "Launch Co-op: Unknown problem", "Code Co-op", MB_OK);
    }
}

void __fastcall Integrator::OnCheckOut (TObject * Sender)
{
    try
    {
        std::vector<std::string> files;
        GetCurrentModuleFiles (files);

        if (files.size () == 0)
          return;

        // string weirdness
        std::vector<const char*> tmp (files.size ());
        for (int i = 0; i < files.size (); ++i)
          tmp [i] = files [i].c_str ();

        SccProxy sccProxy;
        if (!sccProxy.CheckOut (files.size (), &tmp [0], 0))
            throw Win::Exception ("Cannot Checkout the current module files.", tmp [0]);

        UpdateReadOnlyFlagInIDE (files);
    }
    catch (Win::Exception e)
    {
        DisplayEx ("Checkout", e);
    }
    catch (...)
    {
    	Win::ClearError ();
        ::MessageBox (0, "Checkout: Unknown problem", "Code Co-op", MB_OK);
    }
}

void __fastcall Integrator::OnUncheckOut (TObject * Sender)
{
    try
    {
        std::vector<std::string> files;
        GetCurrentModuleFiles (files);

        if (files.size () == 0)
          return;

        // string weirdness
        std::vector<const char*> tmp (files.size ());
        for (int i = 0; i < files.size (); ++i)
          tmp [i] = files [i].c_str ();

        SccProxy sccProxy;
        if (!sccProxy.UncheckOut (files.size (), &tmp[0], 0))
            throw Win::Exception ("Cannot Uncheck out the current module files.", tmp [0]);

        UpdateReadOnlyFlagInIDE (files);
    }
    catch (Win::Exception e)
    {
        DisplayEx ("UncheckOut", e);
    }
    catch (...)
    {
    	Win::ClearError ();
        ::MessageBox (0, "UncheckOut: Unknown problem", "Code Co-op", MB_OK);
    }
}

void __fastcall Integrator::OnCheckIn (TObject * Sender)
{
    try
    {
        std::vector<std::string> files;
        GetCurrentModuleFiles (files);

        if (files.size () == 0)
          return;

        std::string comment;
        std::auto_ptr<TCheckInDlg> checkInDlg (new TCheckInDlg (comment));
        if (checkInDlg->ShowModal () == mrOk)
        {
            // string weirdness
            std::vector<const char*> tmp (files.size ());
            for (int i = 0; i < files.size (); ++i)
              tmp [i] = files [i].c_str ();

            // first, add files that ar not controlled
            std::vector<long> statuses (files.size ());
            SccProxy sccProxy;
            if (!sccProxy.Status (files.size (), &tmp [0], &statuses [0]))
              throw Win::Exception ("Cannot quey statuses of the current module files.", tmp [0]);

            std::vector<char const *> notControlled;
            for (int i = 0; i < files.size (); ++i)
              if (statuses [i] & 1 == 0)
                notControlled.push_back (files [i].c_str ());

            if (notControlled.size () > 0)
              if (!sccProxy.AddFile (notControlled.size (), &notControlled [0], 0))
                throw Win::Exception ("Cannot Add the current module files to source control.", tmp [0]);

            if (!sccProxy.CheckIn (files.size (), &tmp[0], comment.c_str (), 0))
              throw Win::Exception ("Cannot Check In the current module files.", tmp [0]);

            UpdateReadOnlyFlagInIDE (files);
        }
    }
    catch (Win::Exception e)
    {
        DisplayEx ("CheckIn", e);
    }
    catch (...)
    {
    	Win::ClearError ();
        ::MessageBox (0, "CheckIn: Unknown problem", "Code Co-op", MB_OK);
    }
}

void __fastcall Integrator::OnAbout (TObject * Sender)
{
    static char const aboutTxt [] =
                "This is Code Co-op\nServer-less Version Control System\n"
                "\nby Reliable Software.\n\nVisit our web site at:\nwww.ReliSoft.com";
    ::MessageBox (0, aboutTxt, "Code Co-op Version Control System", MB_OK);
}

void Integrator::GetCurrentModuleFiles (std::vector<std::string> & files)
{
    _di_IOTAModuleServices ms;
    HRESULT hr = BorlandIDEServices->QueryInterface (
                            __uuidof (IOTAModuleServices),
                           (void **)&ms);
    if (FAILED (hr))
    {
        // Revisit: use exception that understands HRESULT values
        throw Win::Exception ("Cannot query interface", "IOTAModuleServices");
    }

    if (ms == 0)
      return;

    _di_IOTAModule currentModule = ms->CurrentModule ();
    if (currentModule == 0)
      return;

    files.resize (currentModule->ModuleFileCount);
    for (int i = 0; i < currentModule->ModuleFileCount; ++i)
    {
       files [i].assign (currentModule->ModuleFileEditors [i]->FileName.c_str ());
    }
}

void Integrator::UpdateReadOnlyFlagInIDE (std::vector<std::string> const & files)
{
    // make the current editor's file read-write to IDE
    /*
        first way

        _di_IOTAActionServices as;
        hr = BorlandIDEServices->QueryInterface (
                            __uuidof (IOTAActionServices),
                           (void **)&as);
        if (FAILED (hr))
        {
            // Revisit: use exception that understands HRESULT values
            throw Win::Exception ("Cannot query interface", "IOTAActionServices");
        }

        as->ReloadFile (curFile);
    */

    //   second way, faster but hackish:
    bool isReadOnly = File::IsReadOnly (files [0].c_str ());
    _di_IOTAEditorServices es;
    HRESULT hr = BorlandIDEServices->QueryInterface (
                            __uuidof (IOTAEditorServices),
                           (void **)&es);
    if (FAILED (hr))
    {
        // Revisit: use exception that understands HRESULT values
        throw Win::Exception ("Cannot query interface", "IOTAEditorServices");
    }

    es->TopBuffer->SetIsReadOnly (isReadOnly);
}

void Integrator::DisplayEx (char const * cmd, Win::Exception const & e) const
{
    Msg msg;
    msg << cmd << ": " << e.GetMessage () << std::endl;
    SysMsg sysMsg (e.GetError ());
    if (sysMsg)
        msg << "System tells us: " << sysMsg.Text ();

    std::string objectName (e.GetObjectName ());
    if (!objectName.empty ())
      	msg << "    " << objectName << std::endl;

    ::MessageBox (0, msg.c_str (), "Code Co-op", MB_OK);
}

