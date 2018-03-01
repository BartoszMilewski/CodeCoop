#if !defined (MAILTODLG_H)
#define MAILTODLG_H
// ---------------------------
// (c) Reliable Software, 2005
// ---------------------------
#include "Resource.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>

class MailToCtrl : public Dialog::ControlHandler
{
public:
	MailToCtrl (std::string & emailAddr)
		: Dialog::ControlHandler (IDD_MAIL_TO),
		  _emailAddr (emailAddr)
	{}

    bool OnInitDialog () throw (Win::Exception);
    bool OnApply () throw ();

private:
    Win::Edit	  _emailEdit;
	std::string & _emailAddr;
};

#endif
