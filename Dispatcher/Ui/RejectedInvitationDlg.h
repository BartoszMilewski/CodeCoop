#if !defined (REJECTEDINVITATIONDLG_H)
#define REJECTEDINVITATIONDLG_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "resource.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>

class RejectedInvitationCtrl : public Dialog::ControlHandler
{
public:
	RejectedInvitationCtrl (
			char const * projectName,
			char const * userName,
			char const * computerName,
			char const * explanation)
			: Dialog::ControlHandler (IDD_REJECTED_INVITATION),
		  _projectName (projectName),
		  _userName (userName),
		  _computerName (computerName),
		  _explanation (explanation)
	{}

	bool OnInitDialog () throw (Win::Exception);
private:
	char const * _projectName;
	char const * _userName;
	char const * _computerName;
	char const * _explanation;

	Win::EditReadOnly _projectNameEdit;
	Win::EditReadOnly _userNameEdit;
	Win::EditReadOnly _computerNameEdit;
	Win::StaticText	  _explanationEdit;
};

#endif
