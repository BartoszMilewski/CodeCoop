#if !defined (APPCMDHANDLER_H)
#define APPCMDHANDLER_H
//----------------------------------
//  (c) Reliable Software, 2006
//----------------------------------

class AppCmdHandler
{
public:
	~AppCmdHandler () {}
	virtual bool OnBrowserBackward () { return false; }
	virtual bool OnBrowserForward () { return false; }
};

#endif
