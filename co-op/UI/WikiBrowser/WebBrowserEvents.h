#if !defined (WEBBROWSEREVENTS_H)
#define WEBBROWSEREVENTS_H
//--------------------------------
//  (c) Reliable Software, 2005-06
//--------------------------------
#include <Com/Automation.h>

class WebBrowserEvents
{
public:
	virtual ~WebBrowserEvents () {}
	virtual void BeforeNavigate (std::string const & url, Automation::Bool & cancel) {}
	virtual void DownloadBegin () {}
	virtual void DownloadComplete () {}
	// progress = -1 means completion
	virtual void ProgressChange (long progress, long progressMax) {}
	virtual void DocumentComplete (std::string const & url) {}
};

#endif
