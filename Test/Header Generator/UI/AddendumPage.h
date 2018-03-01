#if !defined (ADDENDUMPAGE_H)
#define ADDENDUMPAGE_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "BasePage.h"

class AddendumCtrl : public BaseController
{
public:
	AddendumCtrl (HeaderDetails & details)
		: BaseController (details),
#pragma warning (disable:4355)
		  _notifyHandler (*this)
#pragma warning (default:4355)
	{}
	void OnInitDialog () throw (Win::Exception);
	bool OnCommand (int ctrlId, int notifyCode) throw (Win::Exception);

	Notify::Handler * GetNotifyHandler (Win::Dow winFrom, int idFrom) throw () { return &_notifyHandler; }

	virtual void RetrieveData ();

private:
	BaseHandler _notifyHandler;
};

#endif
