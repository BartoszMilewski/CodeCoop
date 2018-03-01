#if !defined (INPLACEBROWSER_H)
#define INPLACEBROWSER_H
//-------------------------------
// (c) Reliable Software, 2005-06
//-------------------------------
#include "ClientSite.h"
#include "InPlaceSite.h"
#include "WebBrowserEvents.h"
#include "WebBrowserSite.h"

#include <Com/Automation.h>
#include <Com/Com.h>

class InPlaceBrowser
{
public:
	InPlaceBrowser (Win::Dow::Handle win);
	void Activate ();
	void SetEventHandler (WebBrowserEvents * eventSink);
	void Size (int width, int height) throw ();
	void Navigate (std::string const & loc);
	void Stop () { _browserFace->Stop (); }
private:
	Win::Dow::Handle		_win;
	Automation::SObject		_browserObject;
	Automation::SObjFace<IWebBrowser2, &IID_IWebBrowser2> _browserFace;
	WebBrowserSite			_site;
	int _xPix;
	int _yPix;
};

#endif
