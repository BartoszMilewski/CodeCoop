// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------
#include "Integrator.h"
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
    _separator.reset (NewLine ());
    _about.reset (NewItem ("&About...", 0, false, true, OnAbout, 0, "About"));

    TMenuItem * items [4];
    items [0] = _coopExe.get ();
    items [1] = _checkOut.get ();
    items [2] = _separator.get ();
    items [3] = _about.get ();

    _coopMenu.reset (NewSubMenu ("C&ode Co-op", 0, "CodeCoop", items, 3, true));
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
        _currentFile.clear ();
        // get the name of the current file
        _di_IOTAModuleServices ms;
        HRESULT hr = BorlandIDEServices->QueryInterface (
                            __uuidof (IOTAModuleServices),
                           (void **)&ms);
        if (FAILED (hr))
        {
            // Revisit: use exception that understands HRESULT values
            throw Win::Exception ("Cannot query interface", "IOTAModuleServices");
        }
        if (ms != NULL)
        {
          _di_IOTAModule m = ms->CurrentModule ();
          if (m != NULL)
              _currentFile = m->GetFileName ().c_str ();
        }
        if (_currentFile.empty ())
        {
            _checkOut->Caption = "Check-Out";
            _checkOut->Enabled = false;
        }
        else
        {
            _checkOut->Enabled = true;
            _checkOut->Caption = "Check-Out ";
            _checkOut->Caption += _currentFile.c_str ();
        }
    }
    catch (Win::Exception e)
    {
        DisplayEx ("On Menu Click", e);
    }
    catch (...)
    {
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
    // in BCB together with cpp file check out also
    // 1) corresponding header file
    // 2) corresponding dfm file
    //
    // in Delphi together with pas file check out also
    // 1) corresponding dfm file

    Assert (!_currentFile.empty ());
    try
    {
        // check-out
        char const * curFile = _currentFile.c_str ();

        SccProxy sccProxy;
        if (!sccProxy.CheckOut (1, &curFile, 0))
            throw Win::Exception ("Cannot checkout the current file.", curFile);

        MakeReadWriteInIDE ();

		PathSplitter splitter (curFile);
        bool isCpp = IsNocaseEqual (".cpp", splitter.GetExtension ());
		if (isCpp || IsNocaseEqual (".pas", splitter.GetExtension ()))
		{
            std::string currentHdr;
			std::string currentDfm = splitter.GetDrive ();
            currentDfm += splitter.GetDir ();
            currentDfm += splitter.GetFileName ();
            currentHdr = currentDfm;
            currentDfm += ".dfm";
            currentHdr += ".h";

			char const * dfm = currentDfm.c_str ();
			if (File::Exists (dfm))
            {
        		if (!sccProxy.CheckOut (1, &dfm, 0))
            		throw Win::Exception ("Cannot checkout the current file.", dfm);
            }
            if (!isCpp)
                return;

   	 		char const * hdr = currentHdr.c_str ();
    		if (!File::Exists (hdr))
                return;

           	if (!sccProxy.CheckOut (1, &hdr, 0))
           		throw Win::Exception ("Cannot checkout the current file.", hdr);
        }
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

void Integrator::MakeReadWriteInIDE ()
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
    if (!File::IsReadOnly (_currentFile.c_str ())) // file is r-w on disk
    {
        _di_IOTAEditorServices es;
        HRESULT hr = BorlandIDEServices->QueryInterface (
                            __uuidof (IOTAEditorServices),
                           (void **)&es);
        if (FAILED (hr))
        {
            // Revisit: use exception that understands HRESULT values
            throw Win::Exception ("Cannot query interface", "IOTAEditorServices");
        }

        es->TopBuffer->SetIsReadOnly (False);
    }
}

void __fastcall Integrator::OnAbout (TObject * Sender)
{
    static char const aboutTxt [] =
                "This is Code Co-op\nServer-less Version Control System\n"
                "\nby Reliable Software.\n\nVisit our web site at:\nwww.ReliSoft.com";
    ::MessageBox (0, aboutTxt, "Code Co-op Version Control System", MB_OK);
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

