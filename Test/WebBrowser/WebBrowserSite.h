#if !defined (WEBBROWSERSITE_H)
#define WEBBROWSERSITE_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include <Com/Com.h>

namespace Ole
{
	class ClientSite;
	class InPlaceSite;
}
class WebBrowserEvents;

// Serves as a site for WebBrowser control
class WebBrowserSite: public IUnknown
{
public:
	WebBrowserSite (Win::Dow::Handle winParent);
	~WebBrowserSite ();
	// IUnknown methods
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObject);

	ULONG STDMETHODCALLTYPE AddRef ()
	{
		return ++_refCount;
	}
	ULONG STDMETHODCALLTYPE Release ()
	{
		if(--_refCount == 0)
			delete this;
		return _refCount;
	}
	Win::Dow::Handle GetWindow () const
	{
		return _winParent;
	}
	IUnknown * SetEventHandler (WebBrowserEvents * eventSink);
	Com::IfacePtr<IOleClientSite> QueryOleClientSite ();
private:
	DWORD  _refCount;
	Win::Dow::Handle _winParent;
private:
	class NativeEventSink
	{
	public:
		NativeEventSink () : _sink (0) {}
		void SetSink (WebBrowserEvents * eventSink)
		{
			_sink = eventSink;
		}
		virtual void __stdcall BeforeNavigate2(
			IDispatch *pDisp,
			VARIANT *url,
			VARIANT *Flags,
			VARIANT *TargetFrameName,
			VARIANT *PostData,
			VARIANT *Headers,
			VARIANT_BOOL *Cancel
		);
		virtual void __stdcall DownloadBegin ();
		virtual void __stdcall DownloadComplete ();
		virtual void __stdcall DocumentComplete (IDispatch * pDisp, VARIANT * URL);
		virtual void __stdcall ProgressChange (long prog, long progMax);
	private:
		WebBrowserEvents *	_sink;
	};
private:
	std::auto_ptr<Ole::ClientSite>	_clientSite;
	std::auto_ptr<Ole::InPlaceSite>	_inPlaceSite;
	Com::UnknownPtr					_eventHandler;
	NativeEventSink					_eventSink;
};

#endif
