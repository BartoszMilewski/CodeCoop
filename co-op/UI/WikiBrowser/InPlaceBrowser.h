#if !defined (INPLACEBROWSER_H)
#define INPLACEBROWSER_H
//-------------------------------
// (c) Reliable Software, 2005-06
//-------------------------------
#include "ClientSite.h"
#include "InPlaceSite.h"
#include "WebBrowserEvents.h"
#include "WebBrowserSite.h"

#include <Ctrl/Accelerator.h>
#include <Com/Automation.h>
#include <Com/Com.h>
#include <Mshtml.h>

class BrowserAccel: public Win::Accelerator
{
public:
	BrowserAccel (Com::IfacePtr<IOleInPlaceActiveObject> inPlaceObj)
		: _inPlaceObj (inPlaceObj)
	{
	}
	bool Translate (MSG & msg)
	{
		HRESULT hres = _inPlaceObj->TranslateAccelerator (&msg);
		return hres != S_FALSE;
	}
private:
	Com::IfacePtr<IOleInPlaceActiveObject> _inPlaceObj;
};

class WebBrowserView
{
public:
	virtual void SetFocus () = 0;
};

class InPlaceBrowser: public WebBrowserView
{
public:
	InPlaceBrowser (Win::Dow::Handle win);
	Win::Dow::Handle GetWindow () const { return _win; }
	void Activate ();
	void SetFocus ();
	Win::Accelerator * GetAccelHandler () { return &_accelHandler; }
	int GetVScrollPos ();
	void SetVScrollPos (int pos);
	void SetEventHandler (WebBrowserEvents * eventSink);
	void Move (Win::Rect const & rect) throw ();
	void Navigate (std::string const & loc);
	void Stop () { _browserFace->Stop (); }
private:
	Com::IfacePtr<IHTMLTextContainer> GetDocumentText ();
	Com::IfacePtr<IOleInPlaceActiveObject> GetInPlaceActiveObject ();
private:
	Win::Dow::Handle		_win;
	Automation::SObject		_browserObject;
	Automation::SObjFace<IWebBrowser2, &IID_IWebBrowser2> _browserFace;
	WebBrowserSite			_site;
	BrowserAccel			_accelHandler;
	int _xPix;
	int _yPix;
};

#endif
