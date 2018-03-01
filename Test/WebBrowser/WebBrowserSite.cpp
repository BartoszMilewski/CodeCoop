//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "precompiled.h"
#include "WebBrowserSite.h"
#include "WebBrowserEvents.h"
#include "ClientSite.h"
#include "InPlaceSite.h"
#include <Com/Automation.h>
#include <exdispid.h>

// NativeEventSink methods unpack the arguments and call user-defined sink
void __stdcall WebBrowserSite::NativeEventSink::BeforeNavigate2 (
	IDispatch *pDisp,
	VARIANT *url,
	VARIANT *Flags,
	VARIANT *TargetFrameName,
	VARIANT *PostData,
	VARIANT *Headers,
	VARIANT_BOOL *cancel
)
{
	if (_sink != 0 && url->vt == VT_BSTR)
	{
		Automation::CString urlString (*url);
		Automation::Bool canceller (cancel);
		_sink->BeforeNavigate (urlString.c_str (), canceller);
	}
}

void __stdcall WebBrowserSite::NativeEventSink::DownloadBegin ()
{
	if (_sink)
		_sink->DownloadBegin ();
}

void __stdcall WebBrowserSite::NativeEventSink::DownloadComplete ()
{
	if (_sink)
		_sink->DownloadComplete ();
}

void __stdcall WebBrowserSite::NativeEventSink::DocumentComplete (IDispatch * pDisp, VARIANT * url)
{
	if (_sink != 0 && url->vt == VT_BSTR)
	{
		Automation::CString urlString (*url);
		_sink->DocumentComplete (urlString.c_str ());
	}
}

void __stdcall WebBrowserSite::NativeEventSink::ProgressChange (long prog, long progMax)
{
	if (_sink)
		_sink->ProgressChange (prog, progMax);
}

WebBrowserSite::WebBrowserSite (Win::Dow::Handle winParent)
	: _refCount (1),
	  _winParent (winParent),
	  _eventHandler (0)
{
	_inPlaceSite.reset (new Ole::InPlaceSite (*this));
	_clientSite.reset (new Ole::ClientSite (*this));
}

WebBrowserSite::~WebBrowserSite ()
{}

Com::IfacePtr<IOleClientSite> WebBrowserSite::QueryOleClientSite ()
{
	void * pObj;
	HRESULT hr = QueryInterface (IID_IOleClientSite, &pObj);
	if (FAILED (hr))
		throw Com::Exception (hr, "Query Ole Client Site failed");
	return Com::IfacePtr<IOleClientSite> (static_cast<IOleClientSite *> (pObj));
}

STDMETHODIMP WebBrowserSite::QueryInterface(REFIID iid, void ** ppvObject)
{
	if(ppvObject == 0)
		return E_INVALIDARG;
	*ppvObject = 0;

	if(iid == IID_IUnknown)
		*ppvObject = this;
	else if (iid == IID_IDispatch)
	{
		if (_eventHandler != 0)
			_eventHandler->QueryInterface (iid, ppvObject);
		else
			return E_NOINTERFACE;
	}
	else if (iid == IID_IOleClientSite)
		*ppvObject = _clientSite.get ();
	else if (iid == IID_IOleInPlaceSite)
		*ppvObject = _inPlaceSite.get ();
	else
		return E_NOINTERFACE;
	AddRef();
	return S_OK;
}

// Tables defining type information for Native Web Browser Event Sink

// BeforeNavigate2 arguemtns
PARAMDATA BN2_param [] =
{
	{ L"pDisp", VT_DISPATCH },
	{ L"url", VT_VARIANT | VT_BYREF },
	{ L"Flags", VT_VARIANT | VT_BYREF },
	{ L"TargetFrameName", VT_VARIANT | VT_BYREF },
	{ L"PostData", VT_VARIANT | VT_BYREF },
	{ L"Headers", VT_VARIANT | VT_BYREF },
	{ L"Cancel", VT_BOOL | VT_BYREF },
};

// DocumentComplete arguments
PARAMDATA DC_param [] =
{
	{ L"pDisp", VT_DISPATCH },
	{ L"URL", VT_VARIANT | VT_BYREF}
};

// ProgressChange arguments
PARAMDATA PC_param [] =
{
	{ L"Progress", VT_I4 },
	{ L"ProgressMax", VT_I4 }
};

// Methods implemented by Native Event Handler
METHODDATA methods [] = 
{
	{ 
		L"BeforeNavigate2",
		BN2_param,
		DISPID_BEFORENAVIGATE2,
		0, // vtable offset
		CC_STDCALL,
		7, // argument count
		DISPATCH_METHOD,
		VT_EMPTY
	},
	
	{
		L"DownloadBegin",
		0,
		DISPID_DOWNLOADBEGIN, // id
		1, // vtable
		CC_STDCALL,
		0, // no args
		DISPATCH_METHOD,
		VT_EMPTY
	},
	{
		L"DownloadComplete",
		0,
		DISPID_DOWNLOADCOMPLETE, // id
		2, // vtable
		CC_STDCALL,
		0, // no args
		DISPATCH_METHOD,
		VT_EMPTY
	},
	{
		L"DocumentComplete",
		DC_param,
		DISPID_DOCUMENTCOMPLETE,
		3,
		CC_STDCALL,
		2,
		DISPATCH_METHOD,
		VT_EMPTY
	},
	{
		L"ProgressChange",
		PC_param,
		DISPID_PROGRESSCHANGE,
		4,
		CC_STDCALL,
		2,
		DISPATCH_METHOD,
		VT_EMPTY
	}
};

INTERFACEDATA faceData =
{
	methods,
	5
};

// Plugs in user-defined event sink by creating an IDispatch interface on the fly
IUnknown * WebBrowserSite::SetEventHandler (WebBrowserEvents * eventSink)
{
	_eventSink.SetSink (eventSink);

	ITypeInfo * typeInfo;
	HRESULT hr;
	hr = CreateDispTypeInfo(
		&faceData, LOCALE_SYSTEM_DEFAULT, &typeInfo);
	if(FAILED (hr))
		throw Com::Exception (hr, "Cannot create type info for web browser event handler");

	hr = CreateStdDispatch(
		this,            // Controlling unknown: implements QueryInterface
		&_eventSink,     // Native event handler
		typeInfo,        // Type information describing the event handler
		&_eventHandler); // result: IUnknown that implements IDispatch

	typeInfo->Release();
	if(FAILED (hr))
		throw Com::Exception (hr, "Cannot create standard dispatch");
	return _eventHandler;
}
