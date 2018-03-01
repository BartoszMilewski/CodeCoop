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
	  _accelHandler (GetInPlaceActiveObject ()),
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

void InPlaceBrowser::SetFocus ()
{
	Com::IfacePtr<IOleClientSite> oleClientSite = _site.QueryOleClientSite ();
	Automation::SObjFace<IOleObject, &IID_IOleObject> oleObjFace (_browserObject);

	Win::Rect rect;
	HRESULT hr = oleObjFace->DoVerb (OLEIVERB_UIACTIVATE, NULL, oleClientSite, 0, _win.ToNative (), &rect);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot set focus");
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

void InPlaceBrowser::Move (Win::Rect const & rect) throw ()
{
	Automation::SObjFace<IOleInPlaceObject, &IID_IOleInPlaceObject> oleInPlaceObjFace (_browserObject);
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

Com::IfacePtr<IOleInPlaceActiveObject> InPlaceBrowser::GetInPlaceActiveObject ()
{
	Com::IfacePtr<IOleInPlaceActiveObject> activeObj;
	HRESULT hr = _browserFace->QueryInterface (IID_IOleInPlaceActiveObject, (void **)&activeObj);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot get in-place active object interface");
	return activeObj;
}

Com::IfacePtr<IHTMLTextContainer> InPlaceBrowser::GetDocumentText ()
{
	Com::IfacePtr<IHTMLTextContainer> text;

	Com::IfacePtr<IDispatch> docDispatch;
	HRESULT hr = _browserFace->get_Document (&docDispatch);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot get document IDispatch interface");
	if (docDispatch.IsNull ())
		return text;

	Com::IfacePtr<IHTMLDocument2> doc;
	hr = docDispatch->QueryInterface (IID_IHTMLDocument2, (void **)&doc);
	if (FAILED (hr))
		throw Com::Exception (hr, "Cannot get document interface");
	Com::IfacePtr<IHTMLElement> elem;
	hr = doc->get_body (&elem);
	if (FAILED (hr) || elem.IsNull ())
		return text;

	hr = elem->QueryInterface (IID_IHTMLTextContainer, (void **)&text);
	return text;
}

int InPlaceBrowser::GetVScrollPos ()
{
	Com::IfacePtr<IHTMLTextContainer> doc = GetDocumentText ();
	long pos = 0;
	if (!doc.IsNull ())
		doc->get_scrollTop (&pos);
	return pos;
}

void InPlaceBrowser::SetVScrollPos (int pos)
{
	Com::IfacePtr<IHTMLTextContainer> doc = GetDocumentText ();
	if (!doc.IsNull ())
		doc->put_scrollTop (pos);
}