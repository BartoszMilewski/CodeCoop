#if !defined (ADDRESSPAGE_H)
#define ADDRESSPAGE_H
// ---------------------------
// (c) Reliable Software, 2000
// ---------------------------

#include "BasePage.h"

#include "Ctrl/Button.h"
#include <Ctrl/ComboBox.h>
#include <Ctrl/Edit.h>

class AddressCtrl : public BaseController
{
public:
	AddressCtrl (HeaderDetails & details, bool isForRecip)
		: BaseController (details), 
		  _isForRecip (isForRecip),
#pragma warning (disable:4355)
		  _notifyHandler (*this)
#pragma warning (default:4355)
	{}
	void OnInitDialog () throw (Win::Exception);
	bool OnCommand (int ctrlId, int notifyCode) throw (Win::Exception);

	Notify::Handler * GetNotifyHandler (Win::Dow winFrom, int idFrom) throw () { return &_notifyHandler; }

private:
	void RetrieveData ();
	bool _isForRecip; // the same Ctrl is used both for sender and recipient address
	void SetEmail (char const * email);
	void SetUserId (char const * id);

	BaseHandler _notifyHandler;

	Win::RadioButton _emailKnown;
	Win::ComboBox	 _emailList;
	Win::RadioButton _emailUnknown;
	Win::RadioButton _emailDefine;
	Win::Edit		 _email;

	Win::RadioButton _idKnown;
	Win::ComboBox	 _idList;
	Win::RadioButton _idUnknown;
	Win::RadioButton _idDefine;
	Win::Edit		 _id;
};

#endif
