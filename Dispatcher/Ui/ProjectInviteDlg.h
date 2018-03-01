#if !defined (PROJECTINVITEDLG_H)
#define PROJECTINVITEDLG_H
// ---------------------------
// (c) Reliable Software, 2006
// ---------------------------

#include "resource.h"
#include <Win/Dialog.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>

class JoinProjectData;

class ProjectInviteCtrl : public Dialog::ControlHandler
{
public:
	ProjectInviteCtrl (JoinProjectData & data, std::string const & adminName)
		: Dialog::ControlHandler (IDD_PROJECT_INVITE),
		  _data (data),
		  _adminName (adminName),
		  _isReject (false)
	{}

	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool IsReject () const { return _isReject; }

private:
	Win::EditReadOnly _invitation;
	Win::Edit         _path;
	Win::Button		  _browse;
	Win::Button		  _reject;

	JoinProjectData	  & _data;
	std::string const & _adminName;
	bool			    _isReject;
};

#endif
