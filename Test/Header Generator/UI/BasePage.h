#if !defined (BASEPAGE_H)
#define BASEPAGE_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include <Ctrl/PropertySheet.h>

class HeaderDetails;
class BaseController;

class BaseHandler : public Notify::PageHandler
{
public:
	BaseHandler (BaseController & ctrl)
		: _ctrl (ctrl)
	{}
	void OnSetActive (long & result) throw (Win::Exception);
	void OnKillActive (long & result) throw (Win::Exception);

protected:
	BaseController & _ctrl;
};

class BaseController : public Property::Controller
{
	friend class BaseHandler;
public:
	BaseController (HeaderDetails & details)
		: _details (details)
	{}

	HeaderDetails & GetDetails () { return _details; }
	virtual void RetrieveData () = 0;

protected:
	HeaderDetails & _details;
};

#endif
