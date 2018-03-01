#if !defined (DESTINATIONPAGE_H)
#define DESTINATIONPAGE_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "BasePage.h"

#include "Ctrl/Button.h"
#include <Ctrl/ComboBox.h>
#include <Ctrl/Edit.h>

class DestinationHandler : public BaseHandler
{
public:
	DestinationHandler (BaseController & ctrl)
		: BaseHandler (ctrl)
	{}
	void OnKillActive (long & result) throw (Win::Exception);

};

class DestinationCtrl : public BaseController
{
public:
	DestinationCtrl (HeaderDetails & details)
		: BaseController (details),
#pragma warning (disable:4355)
		  _notifyHandler (*this)
#pragma warning (default:4355)
	{}
	void OnInitDialog () throw (Win::Exception);
	bool OnCommand (int ctrlId, int notifyCode) throw (Win::Exception);
	
	Notify::Handler * GetNotifyHandler (Win::Dow winFrom, int idFrom) throw () { return &_notifyHandler; }

	void RetrieveData ();

private:

	Win::RadioButton _pi;
	Win::RadioButton _inbox;
	Win::RadioButton _outbox;
	Win::ComboBox	 _project;
	Win::ComboBox	 _userId;

	Win::RadioButton _other;
	Win::Edit		 _path;
	Win::Button		 _browse;

	Win::Edit		 _filename;

	DestinationHandler _notifyHandler;
};

#endif
