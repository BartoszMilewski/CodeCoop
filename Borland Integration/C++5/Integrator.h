#if !defined (COOPINTEGRATOR_H)
#define COOPINTEGRATOR_H
// ---------------------------
// (c) Reliable Software, 2002
// ---------------------------
#include <ToolsApi.hpp>
#include <string>
#include <memory>

namespace Win { class Exception; }

class Integrator : public IOTAWizard
{
public:
   Integrator ();
   ~Integrator ();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface (
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject);
    ULONG STDMETHODCALLTYPE AddRef ();
    ULONG STDMETHODCALLTYPE Release ();

    // IOTANotifier
    // Ray Lischner says:
    // Notifier class implements the methods of IOTANotifier as do-nothing stubs. Most Tools API classes do not need these methods,
    // but your derived classes must implement them anyway. You can usually implement them as do-nothing stubs -- you don't need to call
    // the ancestor method because the ancestor methods don't do anything either.
    void __fastcall AfterSave () {}
    void __fastcall BeforeSave () {}
    void __fastcall Destroyed () {}
    void __fastcall Modified () {}

    // IOTAWizard
    void __fastcall Execute ();
    AnsiString __fastcall GetIDString ();
    AnsiString __fastcall GetName ();
    TWizardState __fastcall GetState ();

private:
    std::string _currentFile;
    
    long _refCount;

    static char CodeCoopExpert [];

    void __fastcall OnClickCoopMenu (TObject *Sender);
    void __fastcall OnLaunchCoop (TObject * Sender);
    void __fastcall OnCheckOut (TObject * Sender);
    void __fastcall OnAbout (TObject * Sender);

    void DisplayEx (char const * cmd, Win::Exception const & e) const;
    void MakeReadWriteInIDE ();

    std::auto_ptr<TMenuItem> _coopMenu;
    std::auto_ptr<TMenuItem> _coopExe;
    std::auto_ptr<TMenuItem> _checkOut;
    std::auto_ptr<TMenuItem> _separator;
    std::auto_ptr<TMenuItem> _about;
};

#endif
