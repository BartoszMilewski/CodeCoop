#if !defined (PROJECTADMINDLG_H)
#define PROJECTADMINDLG_H
//-------------------------------------
//  (c) Reliable Software, 1999 - 2005
//-------------------------------------

#include "precompiled.h"
#include "MemberInfo.h"
#include "AdminElection.h"

#include <Ctrl/Edit.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

namespace Project
{
	class Db;
}

//
// Project Admin dialog
//

class ProjectAdminData
{
public:
    ProjectAdminData (Project::Db const & projectDb);

	MemberState GetThisUserState () const { return _thisUserState; }
	bool ProjectHasAdmin () const { return _election.ProjectHasAdmin (); }
	bool IsThisUserAdmin () const { return _thisUserIsAdmin; }
	bool IsNewAdminElected () const { return _election.IsNewAdminElected (); }
	MemberInfo const & GetCurAdmin () const { return _election.GetCurAdmin (); }
	MemberInfo const & GetNewAdmin () const { return _election.GetNewAdmin (); }

private:
	MemberState			_thisUserState;
	bool				_thisUserIsAdmin;
	AdminElection		_election;
};

class ProjectAdminCtrl : public Dialog::ControlHandler
{
public:
    ProjectAdminCtrl (ProjectAdminData * data);

    bool OnInitDialog () throw (Win::Exception);
    bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	Win::EditReadOnly	_name;
	Win::EditReadOnly	_hubId;
	Win::EditReadOnly	_id;
	Win::Button			_newAdmin;
    ProjectAdminData *	_dlgData;
};

#endif
