//------------------------------------
//  (c) Reliable Software, 1998 - 2008
//------------------------------------

#include "precompiled.h"
#include "ScriptRecipients.h"
#include "Addressee.h"
#include "resource.h"

ScriptRecipientsData::ScriptRecipientsData (std::vector<MemberInfo> & addresseeList, GlobalId adminId)
    : _addresseeList (addresseeList),
	  _selected (_addresseeList.size (), false),
	  _adminId (adminId),
	  _scriptId (gidInvalid),
	  _unitType (Unit::Set)
{}

void ScriptRecipientsData::Select (int dataIndex, bool wasSelected, bool isSelected)
{
    if (!wasSelected && isSelected)
    {
		_selected [dataIndex] = true;
    }
    else if (wasSelected && !isSelected)
    {
		_selected [dataIndex] = false;
    }
}

void ScriptRecipientsData::SelectAll ()
{
	std::fill (_selected.begin (), _selected.end (), true);
}

void ScriptRecipientsData::SelectRecipient (UserId id)
{
	unsigned idx = 0;
	unsigned end = _addresseeList.size ();
	while (idx != end)
	{
		if (_addresseeList[idx].Id () == id)
		{
			_selected [idx] = true;
			break;
		}
		++idx;
	}
}

void ScriptRecipientsData::DeselectAll ()
{
	std::fill (_selected.begin (), _selected.end (), false);
}

bool ScriptRecipientsData::IsSelection () const
{
	return std::find (_selected.begin (), _selected.end (), true) != _selected.end ();
}

void ScriptRecipientsData::GetSelection (AddresseeList & addresseeList) const
{
	std::vector<bool>::const_iterator iter = std::find (_selected.begin (), _selected.end (), true);
	while (iter != _selected.end ())
	{
		int index = iter - _selected.begin ();
		MemberInfo const & user = _addresseeList[index];
		Addressee addressee (user.HubId (), user.GetUserId ());
		addresseeList.push_back (addressee);
		++iter;
		iter = std::find (iter, _selected.end (), true);
	}
}

void ScriptRecipientsData::GetSelection (UserIdList & addresseeList) const
{
	std::vector<bool>::const_iterator iter = std::find (_selected.begin (), _selected.end (), true);
	while (iter != _selected.end ())
	{
		int index = iter - _selected.begin ();
		MemberInfo const & user = _addresseeList[index];
		addresseeList.push_back(user.Id());
		++iter;
		iter = std::find (iter, _selected.end (), true);
	}
}

// Windows WM_NOTIFY handlers

RecipientHandler::RecipientHandler (unsigned ctrlId, 
									ScriptRecipientsData * dlgData, 
									ScriptRecipientsCtrl & ctrl, 
									std::vector<int> const & listIndex2Data)
	: Notify::ListViewHandler (ctrlId),
		_ctrl (ctrl),
		_dlgData (dlgData),
		_listIndex2Data (listIndex2Data)
{}

bool RecipientHandler::OnDblClick () throw ()
{
	_ctrl.EndDialog ();
	return true;
}

bool RecipientHandler::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
    int dataIndex = _listIndex2Data [state.Idx ()];
	_dlgData->Select (dataIndex, state.WasSelected (), state.IsSelected ());
	return true;
}

// Script AddresseeList Controller

ScriptRecipientsCtrl::ScriptRecipientsCtrl (ScriptRecipientsData * data)
	: Dialog::ControlHandler (IDD_SCRIPT_RECIPIENTS),
	  _dlgData (data),
#pragma warning (disable: 4355)
	  _notifyHandler (IDC_SCRIPT_RECIPIENTS_LIST, data, *this, _listIndex2Data)
#pragma warning (default: 4355)
{}

Notify::Handler * ScriptRecipientsCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_notifyHandler.IsHandlerFor (idFrom))
		return &_notifyHandler;
	else
		return 0;
}

// Command line
// -Selection_SendScript script:"xx-xxx" [type:"member"] [recipientId:"xx"]
// If type not specified, assume "set"
// If recipientId not specified, assume broadcast
bool ScriptRecipientsCtrl::GetDataFrom (NamedValues const & source)
{
	std::string scriptId = source.GetValue ("script");
	if (scriptId.empty ())
		return false;
	GlobalIdPack idPack (scriptId);
	_dlgData->SetScriptId (idPack);
	std::string type = source.GetValue ("type");
	if (type == "member")
		_dlgData->SetUnitType (Unit::Member);

	std::string recipientId = source.GetValue ("recipientId");
	unsigned long id;
	if (recipientId.empty ())
	{
		_dlgData->SelectAll ();
	}
	else if (HexStrToUnsigned (recipientId.c_str (), id))
	{
		_dlgData->SelectRecipient (id);
	}
	return true;
}

bool ScriptRecipientsCtrl::OnInitDialog () throw (Win::Exception)
{
	_recipientList.Init (GetWindow (), IDC_SCRIPT_RECIPIENTS_LIST);
	_selectAll.Init (GetWindow (), IDC_SCRIPT_RECIPIENTS_SELECT_ALL);
	_deselectAll.Init (GetWindow (), IDC_SCRIPT_RECIPIENTS_DESELECT_ALL);

	_recipientList.AddProportionalColumn (28, "Name");
	_recipientList.AddProportionalColumn (30, "Hub's Email Address");
	_recipientList.AddProportionalColumn (16, "Phone");
	_recipientList.AddProportionalColumn (16, "State");
	_recipientList.AddProportionalColumn (10, "User Id");

	_listIndex2Data.resize (_dlgData->RecipientCount ());
	std::fill (_listIndex2Data.begin (), _listIndex2Data.end (), 0);
	UpdateListView ();
	return true;
}

bool ScriptRecipientsCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
    switch (ctrlId)
    {
    case IDC_SCRIPT_RECIPIENTS_SELECT_ALL:
        if (Win::SimpleControl::IsClicked (notifyCode))
        {
			_dlgData->SelectAll ();
			_recipientList.SelectAll ();
			_recipientList.SetFocus ();
			return true;
        }
        break;
    case IDC_SCRIPT_RECIPIENTS_DESELECT_ALL:
        if (Win::SimpleControl::IsClicked (notifyCode))
        {
			_dlgData->DeselectAll ();
			_recipientList.DeSelectAll ();
			_recipientList.SetFocus ();
			return true;
        }
		break;
    }
    return false;
}

bool ScriptRecipientsCtrl::OnApply () throw ()
{
	EndOk ();
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

void ScriptRecipientsCtrl::UpdateListView ()
{
    _recipientList.ClearRows ();
	RecipientUpdate updateListView (_recipientList, _dlgData->GetAdminId ());
    // Display project member information
	std::for_each (_dlgData->RecipientInfoBegin (), _dlgData->RecipientInfoEnd (), updateListView);
	// Remember display order in the list view
	int memberInfoIndex = 0;
	for (ScriptRecipientsData::recipientIter iter = _dlgData->RecipientInfoBegin (); iter != _dlgData->RecipientInfoEnd (); ++iter)
	{
		int listViewRow = _recipientList.FindItemByData (iter->Id ());
		_listIndex2Data [listViewRow] = memberInfoIndex++;
	}
}
