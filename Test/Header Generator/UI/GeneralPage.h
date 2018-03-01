#if !defined (GENERALPAGE_H)
#define GENERALPAGE_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "BasePage.h"

#include "Ctrl/Button.h"
#include <Ctrl/ComboBox.h>
#include <Ctrl/Edit.h>

class GeneralHandler : public BaseHandler
{
public:
	GeneralHandler (BaseController & ctrl)
		: BaseHandler (ctrl)
	{}
	void OnApply (long & result) throw (Win::Exception);
	void OnReset () throw (Win::Exception);
};

class GeneralCtrl : public BaseController
{
public:
	GeneralCtrl (HeaderDetails & details)
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
	Win::RadioButton _known;
	Win::ComboBox _projectList;
	Win::RadioButton _unknown;
	Win::RadioButton _define;
	Win::Edit _projectName;

	Win::CheckBox _forward;
	Win::CheckBox _defect;
	Win::CheckBox _addendum;

	GeneralHandler _notifyHandler;
};

#endif
