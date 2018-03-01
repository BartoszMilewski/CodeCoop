#if !defined (TOPCOTRL_H)
#define TOPCOTRL_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include "WebBrowserEvents.h"
#include <Win/Controller.h>
#include <Win/Message.h>
#include <Com/Com.h>
namespace Win { class MessagePrepro; }
class InPlaceBrowser;

class EventSink: public WebBrowserEvents
{
public:
	EventSink () : _browser (0) {}
	void Attach (InPlaceBrowser * browser)
	{
		_browser = browser;
	}
	void BeforeNavigate (std::string const & url, Automation::Bool & cancel);
	void DownloadBegin ()
	{
	}
	void DownloadComplete ()
	{
	}
	// progress = -1 means completion
	void ProgressChange (long progress, long progressMax)
	{
	}
	void DocumentComplete (std::string const & url)
	{
	}
private:
	InPlaceBrowser * _browser;
};

class TopCtrl: public Win::Controller
{
public:
	TopCtrl ();
	~TopCtrl ();
	bool OnCreate (Win::CreateData const * create, bool & success) throw ();
	bool OnDestroy () throw ();
	bool OnSize (int width, int height, int flag) throw ();
private:
	Com::UseOle		_useOle;
	EventSink		_eventSink;
	std::auto_ptr<InPlaceBrowser>		_browser;
};
#endif