//-------------------------------
// (c) Reliable Software, 2005-06
//-------------------------------
#include "precompiled.h"
#include "InPlaceBrowser.h"
#include <Win/Geom.h>
#include <Com/Automation.h>
#include <StringOp.h>

InPlaceBrowser::InPlaceBrowser (Win::Dow::Handle win)
	: _win (win),
	  _browserObject (CLSID_WebBrowser, false),
	  _browserFace (_browserObject),
	  _site (win),
	  _xPix (0),
	  _yPix (0)
{
	Activate ();
}

void InPlaceBrowser::Activate ()
{
	Com::IfacePtr<IOleClientSite> oleClientSite = _site.QueryOleClientSite ();
	Automation::SObjFace<IOleObject, &IID_IOleObject> oleObjFace (_browserObject);
	HRESULT hr = oleObjFace->SetClientSite (oleClientSite);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot set client site");

	// Activate the site
	Win::Rect rect;
	hr = oleObjFace->DoVerb (OLEIVERB_INPLACEACTIVATE, NULL, oleClientSite, 0, _win.ToNative (), &rect);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot activate in place");
}

void InPlaceBrowser::SetEventHandler (WebBrowserEvents * eventSink)
{
	IUnknown * handler = _site.SetEventHandler (eventSink);

	DWORD  dwAdviseCookie;
	Com::IfacePtr<IConnectionPoint>  pConnectionPoint;

	Automation::SObjFace<IConnectionPointContainer, &IID_IConnectionPointContainer> connPointContainerFace (_browserObject);
	HRESULT hr = connPointContainerFace->FindConnectionPoint (DIID_DWebBrowserEvents2, &pConnectionPoint);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot find connection point");
	hr = pConnectionPoint->Advise (handler, &dwAdviseCookie);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot advise connection point");
}

void InPlaceBrowser::Size (int width, int height) throw ()
{
	_xPix = width;
	_yPix = height;

	Automation::SObjFace<IOleInPlaceObject, &IID_IOleInPlaceObject> oleInPlaceObjFace (_browserObject);
	Win::Rect rect (10, 10, width - 20, height - 20);
	HRESULT hr = oleInPlaceObjFace->SetObjectRects (&rect, &rect);
	hr; // ignore result
}

void InPlaceBrowser::Navigate (std::string const & loc)
{
	std::wstring wloc (ToWString (loc));
	Automation::BString locator (wloc);
	HRESULT hr = _browserFace->Navigate (*locator.GetPointer (), NULL, NULL, NULL, NULL);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot Navigate", loc.c_str ());
}

