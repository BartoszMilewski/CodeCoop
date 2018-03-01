//-------------------------------------
//  (c) Reliable Software, 1999 -- 2005
//-------------------------------------

#include "precompiled.h"
#include "ProjectAdminDlg.h"
#include "ProjectDb.h"
#include "OutputSink.h"
#include "resource.h"

#include <StringOp.h>

ProjectAdminData::ProjectAdminData (Project::Db const & projectDb)
	: _thisUserState (projectDb.GetMemberState (projectDb.GetMyId ())),
	  _thisUserIsAdmin (projectDb.IsProjectAdmin ()),
	  _election (projectDb)
{}

ProjectAdminCtrl::ProjectAdminCtrl (ProjectAdminData * data)
	: Dialog::ControlHandler (IDD_PROJECT_ADMIN),
	  _dlgData (data)
{}

bool ProjectAdminCtrl::OnInitDialog () throw (Win::Exception)
{
	_name.Init (GetWindow (), IDC_ADMIN_NAME);
	_hubId.Init (GetWindow (), IDC_ADMIN_EMAIL);
	_id.Init (GetWindow (), IDC_ADMIN_ID);
	_newAdmin.Init (GetWindow (), IDC_NEW_ADMIN);

	if (_dlgData->ProjectHasAdmin ())
	{
		MemberInfo const & curAdmin = _dlgData->GetCurAdmin ();
		_name.SetString (curAdmin.Name ().c_str ());
		_hubId.SetString (curAdmin.HubId ().c_str ());
		_id.SetString (ToHexString (curAdmin.Id ()).c_str ());
	}
	else
	{
		_name.SetString ("Project without administrator");
	}
	MemberState myState = _dlgData->GetThisUserState ();
	if (myState.IsReceiver ())
		_newAdmin.Disable ();	// Receivers cannot perform emergency administrator election
	return true;
}

bool ProjectAdminCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_NEW_ADMIN && Win::SimpleControl::IsClicked (notifyCode))
	{
		MemberState thisUserState = _dlgData->GetThisUserState ();
		if (thisUserState.IsObserver ())
		{
			TheOutput.Display ("Change your state from observer to voting member and then\n"
							   "try emergency project administrator election.",
							   Out::Information, GetWindow ());
		}
		else
		{
			Assert (thisUserState.IsVoting ());
			if (_dlgData->IsThisUserAdmin ())
			{
				TheOutput.Display ("You are the project administrator.  To select a new administrator\n"
								   "use the Project>Members dialog.",
								    Out::Information, GetWindow ());
			}
			else if (_dlgData->IsNewAdminElected ())
			{
				MemberInfo const & newAdmin = _dlgData->GetNewAdmin ();
				std::string info ("Emergency Administrator Election.\n\nThe new project administrator will be:\n\n");
				MemberNameTag nameTag (newAdmin.Name (), newAdmin.Id ());
				info += nameTag;
				info += ", hub's email address: ";
				info += newAdmin.HubId ();
				info += "\n\nDo you want to proceed?";
				Out::Answer userChoice = TheOutput.Prompt (info.c_str (),
															Out::PromptStyle (Out::YesNo, Out::Yes, Out::Question));
				if (userChoice == Out::Yes)
				{
					EndOk ();
				}
			}
			else
			{
				TheOutput.Display ("Emergency Administrator Election.\n\n"
								   "Cannot elect new project administrator, because there are no voting members.",
								   Out::Information, GetWindow ());
			}
		}
		GetWindow ().SetFocus ();
		return true;
	}
    return false;
}

bool ProjectAdminCtrl::OnApply () throw ()
{
	EndCancel ();
	return true;
}