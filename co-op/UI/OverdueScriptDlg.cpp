//------------------------------------
//  (c) Reliable Software, 2004 - 2005
//------------------------------------

#include "precompiled.h"
#include "OverdueScriptDlg.h"
#include "ProjectMembers.h"
#include "ScriptProps.h"
#include "Resource.h"

OverdueScriptCtrl::OverdueScriptCtrl (ScriptProps const & data, 
									  std::set<UserId> & membersToRemove,
									  bool & showDetails)
	: Dialog::ControlHandler (IDD_OVERDUE_SCRIPT),
	  Notify::ListViewHandler(IDC_OVERDUE_MEMBER_LIST),
	  _dlgData (data),
	  _membersToRemove(membersToRemove),
	  _showDetails(showDetails)
{}

bool OverdueScriptCtrl::OnInitDialog () throw (Win::Exception)
{
	_removeButton.Init(GetWindow(), IDC_BUTTON_REMOVE);
	_memberListing.Init (GetWindow (), IDC_OVERDUE_MEMBER_LIST);

	_memberListing.AddProportionalColumn (38, "Name");
	_memberListing.AddProportionalColumn (48, "Hub's Email Address");
	_memberListing.AddProportionalColumn (10, "User Id");
	ScriptProps::MemberSequencer seq (_dlgData);
	Assert (!seq.AtEnd ());
	for ( ; !seq.AtEnd (); seq.Advance ())
	{
		int row = _memberListing.AppendItem (seq.GetName (), seq.GetUserId());
		_memberListing.AddSubItem (seq.GetHubId (), row, 1);
		_memberListing.AddSubItem (seq.GetStrId (), row, 2);
	}
	return true;
}

bool OverdueScriptCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (id == IDC_BUTTON_DETAILS && Win::SimpleControl::IsClicked (notifyCode))
	{
		_showDetails = true;
		EndOk();
		return true;
	}
	if (id == IDC_BUTTON_REMOVE)
	{
		Assert(_memberListing.GetSelectedCount() != 0);
		int i = _memberListing.GetFirstSelected();
		do {
			UserId uid = _memberListing.GetItemParam(i);
			_membersToRemove.insert(uid);
			i = _memberListing.GetNextSelected(i);
		} while (i != -1);
		EndOk();
		return true;
	}
	return false;
}

bool OverdueScriptCtrl::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (_memberListing.GetSelectedCount() != 0)
		_removeButton.Enable();
	else
		_removeButton.Disable();

	return true;
}
