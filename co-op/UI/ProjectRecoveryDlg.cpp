//----------------------------------
// (c) Reliable Software 2007 - 2008
//----------------------------------

#include "precompiled.h"
#include "ProjectRecoveryDlg.h"
#include "ProjectDb.h"
#include "OutputSink.h"
#include "Resource.h"

ProjectRecoveryData::ProjectRecoveryData (Project::Db const & projectDb,
										  std::string const & caption,
										  bool isRestoreFromBackup)
	: _adminId (projectDb.GetAdminId ()),
	  _selectedRecipientId (gidInvalid),
	  _isRestoreFromBackup (isRestoreFromBackup),
	  _dontBlockCheckin (false),
	  _alwaysSendToAdmin (false),
	  _caption (caption),
      _votingMembers(projectDb.RetrieveVotingMemberList())
{
	if (_votingMembers.size () == 0)
		return; // Caller make sure not to show the dialog

	// Select the administrator, or take the first member from the list
	if (_adminId == gidInvalid || _adminId == projectDb.GetMyId ())
		_selectedRecipientId = _votingMembers[0].Id ();
	else
		_selectedRecipientId = _adminId;
}

void ProjectRecoveryData::SelectRecipient (unsigned idx)
{
	Assert (idx < _votingMembers.size ());
	_selectedRecipientId = _votingMembers[idx].Id ();
}

RecipientListHandler::RecipientListHandler (unsigned ctrlId, 
											ProjectRecoveryData & dlgData, 
											ProjectRecoveryRecipientsCtrl & ctrl, 
											std::vector<int> const & listIndex2Data)
	: Notify::ListViewHandler (ctrlId),
	  _ctrl (ctrl),
	  _dlgData (dlgData),
	  _listIndex2Data (listIndex2Data)
{}

bool RecipientListHandler::OnDblClick () throw ()
{
	_ctrl.OnApply ();
	return true;
}

bool RecipientListHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.IsSelected ())
		_dlgData.SelectRecipient (_listIndex2Data [state.Idx ()]);
	else if (state.LostSelection())
		_dlgData.ClearSelection();

	return true;
}

// Script AddresseeList Controller

ProjectRecoveryRecipientsCtrl::ProjectRecoveryRecipientsCtrl (ProjectRecoveryData & data)
	: Dialog::ControlHandler (IDD_VERIFICATION_REQUEST_RECIPIENTS),
	  _dlgData (data),
#pragma warning (disable: 4355)
	  _notifyHandler (IDC_RECIPIENTS_LIST, data, *this, _listIndex2Data)
#pragma warning (default: 4355)
{}

Notify::Handler * ProjectRecoveryRecipientsCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_notifyHandler.IsHandlerFor (idFrom))
		return &_notifyHandler;
	else
		return 0;
}

bool ProjectRecoveryRecipientsCtrl::OnInitDialog () throw (Win::Exception)
{
	_recipientList.Init (GetWindow (), IDC_RECIPIENTS_LIST);
	_caption.Init (GetWindow (), IDC_DLG_CAPTION);
	_optionCheckbox.Init (GetWindow (), IDC_OPTION_CHECKBOX);

	_caption.SetText (_dlgData.GetCaption ());
	if (_dlgData.IsRestoreFromBackup ())
	{
		_optionCheckbox.SetText ("Always send request to the project administrator and don't show me this dialog again");
		if (_dlgData.IsAlwaysSendToAdmin())
			_optionCheckbox.Check();
	}
	else
		_optionCheckbox.SetText ("Don't block check-ins (not recommended)");
	_recipientList.AddProportionalColumn (28, "Name");
	_recipientList.AddProportionalColumn (30, "Hub's Email Address");
	_recipientList.AddProportionalColumn (16, "Phone");
	_recipientList.AddProportionalColumn (16, "State");
	_recipientList.AddProportionalColumn (10, "User Id");

	_listIndex2Data.resize (_dlgData.RecipientCount ());
	std::fill (_listIndex2Data.begin (), _listIndex2Data.end (), 0);
	UpdateListView ();
	int listViewRow = _recipientList.FindItemByData (_dlgData.GetSelectedRecipientId ());
	_recipientList.Select (listViewRow);
	return true;
}

bool ProjectRecoveryRecipientsCtrl::OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception)
{
	if (!_dlgData.IsRestoreFromBackup())
		return true;

	if (id == IDC_OPTION_CHECKBOX && _optionCheckbox.IsChecked())
	{
		UserId adminId = _dlgData.GetAdminId ();
		if (adminId != gidInvalid)
		{
			int listViewRow = _recipientList.FindItemByData (adminId);
			if (listViewRow != -1)
			{
				_recipientList.DeSelectAll();
				_recipientList.Select(listViewRow);
			}
		}
	}
	return true;
}

bool ProjectRecoveryRecipientsCtrl::OnApply () throw ()
{
	if (_dlgData.IsRestoreFromBackup ())
	{
		_dlgData.SetAlwaysSendToAdmin (_optionCheckbox.IsChecked ());
		
		if (_dlgData.GetSelectedRecipientId () == gidInvalid)
		{
			TheOutput.Display ("Please select the most available member.", Out::Information, GetWindow ());
		}
		else
		{
			EndOk ();
		}
	}
	else
	{
		if (_dlgData.GetSelectedRecipientId () == gidInvalid)
		{
			TheOutput.Display ("Please select the most available member.", Out::Information, GetWindow ());
		}
		else
		{
			_dlgData.SetDontBlockCheckin (_optionCheckbox.IsChecked ());
			EndOk ();
		}
	}
	return true;
}

class RecipientUpdate
{
public:
	RecipientUpdate (Win::ReportListing & listing, GlobalId adminId)
		: _listing (listing),
		  _adminId (adminId)
	{}

	void operator() (MemberInfo const & member)
	{
		MemberDescription const & description = member.Description ();
		int row = _listing.AppendItem (description.GetName ().c_str (), member.Id ());
		_listing.AddSubItem (description.GetHubId ().c_str (), row, colHubId);
		_listing.AddSubItem (description.GetComment ().c_str (), row, colPhone);
		if (_adminId == member.Id ())
			_listing.AddSubItem ("Administrator", row, colState);
		else
			_listing.AddSubItem (member.GetStateDisplayName (), row, colState);
		std::string idStr (ToHexString (member.Id ()));
		_listing.AddSubItem (idStr.c_str (), row, colId);
	}

private:
	enum
	{
		colName,
		colHubId,
		colPhone,
		colState,
		colId
	};

	Win::ReportListing &	_listing;
	GlobalId				_adminId;
};

void ProjectRecoveryRecipientsCtrl::UpdateListView ()
{
	_recipientList.ClearRows ();
	RecipientUpdate updateListView (_recipientList, _dlgData.GetAdminId ());
	// Display project member information
	std::for_each (_dlgData.RecipientInfoBegin (), _dlgData.RecipientInfoEnd (), updateListView);
	// Remember display order in the list view
	int memberInfoIndex = 0;
	for (ProjectRecoveryData::RecipientIter iter = _dlgData.RecipientInfoBegin ();
		 iter != _dlgData.RecipientInfoEnd ();
		 ++iter)
	{
		int listViewRow = _recipientList.FindItemByData (iter->Id ());
		_listIndex2Data [listViewRow] = memberInfoIndex++;
	}
}
