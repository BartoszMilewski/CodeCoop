//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "precompiled.h"
#include "ProjectMembers.h"
#include "License.h"
#include "ProjectSeats.h"
#include "ProjectDb.h"
#include "Catalog.h"
#include "Prompter.h"
#include "OutputSink.h"
#include "AppInfo.h"
#include "resource.h"
#include "DistributorInfo.h"
#include "ActivityLog.h"

#include <StringOp.h>
#include <Win/Utility.h>
#include <Ctrl/Output.h>
#include <Com/Shell.h>

ProjectMembersData::ProjectMembersData (Project::Db const & projectDb, unsigned int trialDaysLeft)
	: _thisUserIdx (-1),
	  _trialDaysLeft (trialDaysLeft),
      _memberData(projectDb.RetrieveMemberList())
{
	_infoChanged.resize (_memberData.size (), 0);
	_missingSeats = CountMissingSeats ();
	for (unsigned int i = 0; i < _memberData.size (); ++i)
	{
		if (_memberData [i].Id () == projectDb.GetMyId ())
		{
			_thisUserIdx = i;
			_thisUserState = _memberData [i].State ();
			break;
		}
	}
}

unsigned int ProjectMembersData::ElectNewAdmin (unsigned int currentAdminIdx)
{
	unsigned int newAdminIdx = -1;
	MemberState currentAdminState = _memberData [currentAdminIdx].State ();
	StateObserver tmpState (currentAdminState);
	_memberData [currentAdminIdx].SetState (tmpState);
	memberIter iter = ElectAdmin (_memberData.begin (), _memberData.end ());
	if (iter != _memberData.end ())
		newAdminIdx = iter - _memberData.begin ();

	_memberData [currentAdminIdx].SetState (currentAdminState);
	return newAdminIdx;
}

void ProjectMembersData::SetNewMemberInfo (int index, MemberInfo && newMemberInfo)
{
    _memberData[index] = std::move(newMemberInfo);
    _infoChanged [index] = 1;
	_missingSeats = CountMissingSeats ();
}

void ProjectMembersData::ChangeMemberState (int index, MemberState newState)
{
	_memberData[index].SetState(newState);
    _infoChanged [index] = 1;
	_missingSeats = CountMissingSeats ();
}

unsigned int ProjectMembersData::FullMemberCount () const
{
	return std::count_if (_memberData.begin (), _memberData.end (), [](MemberInfo const & info)
    {
            return !info.IsReceiver();
    });
}

int ProjectMembersData::CmpRows (int rowX, int rowY, int sortCol) const
{
	Assert (0 <= rowX && rowX < _memberData.size ());
	Assert (0 <= rowY && rowY < _memberData.size ());
	// Sort columns
	//		0 - member name
	//		1 - member hub id
	//		2 - member state
	//		3 - member id
	// Return Value     Description
	//    < 0           rowX less than rowY
	//      0           rowX identical to rowY
	//    > 0           rowX greater than rowY
	int cmpResult = 0;
	MemberInfo const & memberX = _memberData [rowX];
	MemberInfo const & memberY = _memberData [rowY];
	switch (sortCol)
	{
	case 0:	// member name
		cmpResult = NocaseCompare (memberX.Name (), memberY.Name ());
		break;
	case 1:	// member hub id
		cmpResult = NocaseCompare (memberX.HubId (), memberY.HubId ());
		break;
	case 2:	// member state
		{
			MemberState stateX = memberX.State ();
			MemberState stateY = memberY.State ();
			if (!stateX.IsEqual (stateY))
			{
				if (stateX < stateY)
					cmpResult = -1;
				else
					cmpResult = 1;
			}
		}
		break;

	case 3:	// member id
		if (memberX.Id () < memberY.Id ())
			cmpResult = -1;
		else if (memberX.Id () > memberY.Id ())
			cmpResult = 1;
		break;

	default:
		Assert (!"Illegal sort column in the Project>Members dialog");
		break;
	}
	return cmpResult;
}

bool ProjectMembersData::CanEditMember (unsigned int dataIndex) const
{
	if (_thisUserIdx == -1)
	{
		// No current user among project members, probably removed by admin
		return false;
	}
	else
	{
		return dataIndex == _thisUserIdx	||	// This user can edit its own data
			   _thisUserState.IsAdmin ()	||	// Administrator can edit anyone data
			   !_thisUserState.IsVerified ();	// Anyone can edit unverified user data
	}
}

bool ProjectMembersData::CanMakeVoting (int selectedMember, bool & notEnoughSeats) const
{
	notEnoughSeats = false;
#if defined BETA
	// During Beta we always allow change to voting member
	return true;
#else
	MemberInfo const & selectedInfo = _memberData [selectedMember];
	Assert (selectedInfo.State ().IsObserver () || selectedInfo.State ().IsAdmin ());
	// Project member can change his state to voting member when he has a valid license and there are
	// free project seats available or when we are during the trial period
	if (_trialDaysLeft > 0)
		return true;

	if (!selectedInfo.Description ().IsLicensed ())
		return false;

	Project::Seats seats (_memberData, selectedInfo.Description ().GetLicense ());
	notEnoughSeats = !seats.IsEnoughLicenses ();
	return !notEnoughSeats;
#endif;
}

bool ProjectMembersData::ChangesDetected () const
{
	return std::find (_infoChanged.begin (), _infoChanged.end (), 1) != _infoChanged.end ();
}

int ProjectMembersData::CountMissingSeats () const
{
	// Count missing project seats
	Assert (_memberData.size () != 0);
	Project::Seats seats (_memberData, _memberData[0].Description ().GetLicense ());
	return seats.GetMissing ();
}

bool ProjectMembersData::CanDisplayLicense (int idx)
{
	return IsThisUser (idx) 
		|| (!ThisUserState ().IsObserver () && !ThisUserState ().IsReceiver ());
}

//---------------
// Member display
//---------------

int const MemberDisplay::_iconIds [imageLast] =
{
	IDI_ADMINISTRATOR,
	IDI_PROJECT_MEMBER,
	IDI_THIS_MEMBER,
	IDI_VERIFIED_MEMBER,
	IDI_THIS_VERIFIED_MEMBER,
};

MemberDisplay::MemberDisplay (ProjectMembersData const & memberList,
							  ProjectMembersCtrl & dlgCtrl,
							  Win::Dow::Handle winParent)
	: Notify::ListViewHandler (IDC_PROJECT_MEMBERS_LIST),
	  _memberImages (16, 16, imageLast),
	  _sortColumn (0),
	  _isAscending (true),
	  _memberList (memberList),
	  _selectedMemberIdx (_memberList.GetThisUserIdx ()),
	  _dlgCtrl (dlgCtrl)
{
	_sortVector.reserve (_memberList.MemberCount ());
	unsigned int rowCount = _memberList.MemberCount ();
	for (unsigned int i = 0; i < rowCount; i++)
		_sortVector.push_back (i);

	for (int i = 0; i < imageLast; ++i)
	{
		Icon::SharedMaker icon (16, 16);
		_memberImages.AddIcon (icon.Load (winParent.GetInstance (), _iconIds [i]));
	}
	_memberImages.SetOverlayImage (imageOverlayAdministrator, overlayAdministrator);
}

MemberDisplay::~MemberDisplay ()
{
	_displayList.SetImageList ();
}

void MemberDisplay::Init (Win::Dow::Handle dlgWin, unsigned ctrlId)
{
	_displayList.Init (dlgWin, ctrlId);
	_displayList.AddProportionalColumn (33, "Name");
	_displayList.AddProportionalColumn (35, "Hub's Email Address");
	_displayList.AddProportionalColumn (23, "State");
	_displayList.AddProportionalColumn (8, "Id");
	_displayList.SetImageList (_memberImages);
}

bool MemberDisplay::OnItemChanged (Win::ListView::ItemState & state) throw ()
{
	if (state.GainedSelection ())
	{
		_selectedMemberIdx = _sortVector [state.Idx ()];
		_dlgCtrl.Select (_selectedMemberIdx);
	}
	return true;
}

bool MemberDisplay::OnColumnClick (int col) throw ()
{
	if (_sortColumn == col)
	{
		_isAscending = !_isAscending;
	}
	else
	{
		_sortColumn = col;
		if (col == 2)
			_isAscending = false;
		else
			_isAscending = true;
	}
	Refresh ();
	return true;
}

void MemberDisplay::Refresh ()
{
	std::stable_sort (_sortVector.begin (),
					  _sortVector.end (),
					  SortPredicate (_memberList, _sortColumn, _isAscending));

	_displayList.ClearRows ();
	int selectedRow = 0;
	for (unsigned int i = 0; i < _sortVector.size (); ++i)
	{
		int memberRow = _sortVector [i];
		if (memberRow == _selectedMemberIdx)
			selectedRow = i;
		MemberInfo const & member = _memberList.GetMemberInfo (memberRow);
		MemberDescription const & description = member.Description ();
		Win::ListView::Item item;
		item.SetText (description.GetName ().c_str ());
		item.SetParam (member.Id ());
		MemberState state = member.State ();
		int overlay = 0;
		if (state.IsAdmin ())
		{
			overlay = overlayAdministrator;
		}

		int image = state.IsVerified () 
			? imageVerifiedMember 
			: imageMember;
		if (member.Id () == _memberList.GetThisUserId ())
		{
			image = state.IsVerified () 
				? imageThisVerifiedMember 
				: imageThisMember;
		}
		item.SetIcon (image);
		item.SetOverlay (overlay);
		int row = _displayList.AppendItem (item);
		_displayList.AddSubItem (description.GetHubId ().c_str (), row, colHubId);
		_displayList.AddSubItem (member.GetStateDisplayName (), row, colState);
		std::string idString = ::ToHexString (member.Id ());
		_displayList.AddSubItem (idString.c_str (), row, colId);
	}
	_displayList.Select (selectedRow);
	_displayList.SetFocus (selectedRow);
	_displayList.EnsureVisible (selectedRow);
}

bool MemberDisplay::SortPredicate::operator ()(int rowX, int rowY)
{
	// Return Value     Description
	//    < 0           rowX less than rowY
	//      0           rowX identical to rowY
	//    > 0           rowX greater than rowY
	int cmpResult = _memberList.CmpRows (rowX, rowY, _sortCol);
	bool predicateValue = false;
	if (cmpResult != 0)
	{
		if (_isAscending)
			predicateValue = cmpResult < 0;
		else
			predicateValue = cmpResult > 0;
	}
	return predicateValue;
}

//-----------
// Controller
//-----------

// aux
bool IsIn (std::string const & str, std::set<std::string> const & strSet)
{
	return strSet.find (str) != strSet.end ();
}

// -Project_Members userId:"id" [name:"name" | state:"[voting/observer/removed/admin]]"
bool ProjectMembersCtrl::GetDataFrom (NamedValues const & source)
{
	std::string userIdStr     = source.GetValue ("userid");
	std::string newName       = source.GetValue ("name");
	std::string newStateName  = source.GetValue ("state");

	// convert the user id into the  position on the _memberData list used in the dialog
	unsigned long userId;
	if (userIdStr == "-1")
		userId = _dlgData.GetThisUserId ();
	else
	{
		if (!HexStrToUnsigned (userIdStr.c_str (), userId))
			throw Win::InternalException ("Invalid member id", userIdStr.c_str ());
	}

	_selectedMemberIdx = -1;
	ProjectMembersData::memberIter it = _dlgData.MemberInfoBegin ();
	for (;
		it != _dlgData.MemberInfoEnd ();
		++it)
	{
		++_selectedMemberIdx;
		MemberInfo const & memberInfo = *it;
		if (memberInfo.Id () == userId)
			break;
	}
	if (it == _dlgData.MemberInfoEnd ())
		throw Win::InternalException ("Member not found in the project.");

	if (!_dlgData.CanEditMember (_selectedMemberIdx))
		throw Win::InternalException ("You are not allowed to edit this member.");

	std::set<std::string> allowedStates;
	GetAllowedStateChanges (allowedStates);
	if (!IsIn (newStateName, allowedStates))
		throw Win::InternalException ("Specified state change is not allowed.");

	MemberInfo const oldInfo (_dlgData.GetMemberInfo (_selectedMemberIdx));
	MemberState const oldState = oldInfo.State ();
	MemberInfo newInfo (oldInfo);
	// Determine new member state

	if (!newStateName.empty ())
	{
		if (newStateName == "voting")
			newInfo.MakeVoting ();
		else if (newStateName == "observer")
			newInfo.MakeObserver ();
		else if (newStateName == "removed")
			newInfo.Defect (false);
		else if (newStateName == "admin")
			newInfo.MakeAdmin ();
	}

	MemberState newState (newInfo.State ());
	if (!newState.IsEqual (oldState))
	{
		if (!VerifyMemberStateChange (oldState, newState))
			throw Win::InternalException ("Specified state change is not allowed.");
	}

	if (!newName.empty ())
	{
		newInfo.SetName (newName);
	}

	_dlgData.SetNewMemberInfo (_selectedMemberIdx, std::move(newInfo));

	return true;
}

void ProjectMembersCtrl::GetAllowedStateChanges (std::set<std::string> & allowedStates) const
{
	bool const isChangingHimself = _dlgData.IsThisUser (_selectedMemberIdx);
	if (_dlgData.IsAdmin (_dlgData.GetThisUserIdx ()))
	{
		// Project administrator state change options
		if (_dlgData.IsReceiver (_selectedMemberIdx))
		{
			// Administrator can only remove receiver from the project
			allowedStates.insert ("removed");
		}
		else
		{
			// Administrator can change state of the full member
			allowedStates.insert ("admin");
			if (_dlgData.FullMemberCount () > 1)
			{
				allowedStates.insert ("voting");
				allowedStates.insert ("observer");
				if (!isChangingHimself)
				{
					// Administrator cannot remove himself from the project using this dialog -- he/she has to defect
					allowedStates.insert ("removed");
				}
			}
			else
			{
				// Only one project member -- no state changes allowed
			}
		}
	}
	else if (isChangingHimself)
	{
		// Regular project member state change options
		if (_dlgData.IsReceiver (_selectedMemberIdx))
		{
			// Receiver cannot change his/her state
		}
		else
		{
			allowedStates.insert ("voting");
			allowedStates.insert ("observer");
		}
	}
	else
	{
		// Regular project member edits unverified project member -- allow only state change
		Assert (!_dlgData.IsReceiver (_selectedMemberIdx));
		allowedStates.insert ("voting");
		allowedStates.insert ("observer");
		allowedStates.insert ("removed");
	}
}


//----------------------------------
// Project members dialog controller
//----------------------------------

ProjectMembersCtrl::ProjectMembersCtrl (ProjectMembersData & data, 
										Catalog & catalog, 
										ActivityLog & log,
										Win::Dow::Handle winParent)
	: Dialog::ControlHandler (IDD_PROJECT_MEMBERS),
	  _catalog (catalog),
	  _log (log),
	  _distributorPool (catalog),
	  _dlgData (data),
	  _selectedMemberIdx (data.GetThisUserIdx ()),
#pragma warning (disable:4355)
	  _displayList (data, *this, winParent)
#pragma warning (default:4355)
{}

Notify::Handler * ProjectMembersCtrl::GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
{
	if (_displayList.IsHandlerFor (idFrom))
		return &_displayList;
	else
		return 0;
}

bool ProjectMembersCtrl::OnInitDialog () throw (Win::Exception)
{
	Win::Dow::Handle dlgWin = GetWindow ();
	_displayList.Init (dlgWin, IDC_PROJECT_MEMBERS_LIST);
	_name.Init (dlgWin, IDC_EDIT_MEMBER_NAME);
	_hubId.Init (dlgWin, IDC_EDIT_MEMBER_EMAIL);
	_phone.Init (dlgWin, IDC_EDIT_MEMBER_PHONE);
	_voting.Init (dlgWin, IDC_EDIT_MEMBER_VOTING);
	_observer.Init (dlgWin, IDC_EDIT_MEMBER_OBSERVER);
	_removed.Init (dlgWin, IDC_EDIT_MEMBER_REMOVED);
	_admin.Init (dlgWin, IDC_EDIT_MEMBER_ADMIN);
	_checkoutNotification.Init (dlgWin, IDC_START_CHECKOUT_NOTIFICATIONS);
	_distributor.Init (dlgWin, IDC_DISTRIBUTOR);
	_applyChanges.Init (dlgWin, IDC_PROJECT_MEMBERS_EDIT);
	_assignLicense.Init (dlgWin, IDB_LICENSE);
	_licenseDisplay.Init (dlgWin, IDC_LICENSE);
	_licensePool.Init (dlgWin, IDC_LICENSE_POOL);
	_assignText.Init (dlgWin, IDC_ASSIGN);

	Assert (_dlgData.GetThisUserIdx () != -1);
	_displayList.Refresh ();
	_applyChanges.Disable ();
	return true;
}

bool ProjectMembersCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	switch (ctrlId)
	{
	case IDC_EDIT_MEMBER_NAME:
		if (_name.IsChanged (notifyCode))
		{
			_applyChanges.Enable ();
		}
		return true;
	case IDC_EDIT_MEMBER_EMAIL:
		if (_hubId.IsChanged (notifyCode))
		{
			_applyChanges.Enable ();
		}
		return true;
	case IDC_EDIT_MEMBER_PHONE:
		if (_phone.IsChanged (notifyCode))
		{
			_applyChanges.Enable ();
		}
		return true;
	case IDC_EDIT_MEMBER_VOTING:
	case IDC_EDIT_MEMBER_OBSERVER:
	case IDC_EDIT_MEMBER_REMOVED:
	case IDC_EDIT_MEMBER_ADMIN:
	case IDC_START_CHECKOUT_NOTIFICATIONS:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			MemberState oldState = _dlgData.GetMemberInfo (_selectedMemberIdx).State ();
			MemberState newState (oldState);
			GetSelectedMemberState (newState);
			if (!newState.IsEqual (oldState))
				_applyChanges.Enable ();
			else
				_applyChanges.Disable ();
		}
		return true;
	case IDC_PROJECT_MEMBERS_EDIT:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			ApplyChanges ();
			return true;
		}
		break;
	case IDB_LICENSE:
		if (Win::SimpleControl::IsClicked (notifyCode))
		{
			AssignReceiverLicense ();
			return true;
		}
		break;
	}
    return false;
}

bool ProjectMembersCtrl::OnApply () throw ()
{
	if (_applyChanges.IsEnabled ())
		ApplyChanges ();
	EndOk ();
	return true;
}

void ProjectMembersCtrl::AssignReceiverLicense ()
{
	if (!_distributorPool.empty () // I have licenses
		&& _dlgData.IsReceiver (_selectedMemberIdx) // he's receiver
		&& _dlgData.IsAdmin (_dlgData.GetThisUserIdx ())) // I'm admin
	{
		_newLicense = _distributorPool.NewLicense (_log);
		ApplyChanges ();
		_newLicense.clear ();
		_cancel.Disable (); // too late to cancel
	}
	else
	{
		Win::Dow::Handle appWnd = TheAppInfo.GetWindow ();
		int errCode = ShellMan::Open (appWnd, DistributorPurchaseLink);
		if (errCode != -1)
		{
			std::string msg = ShellMan::HtmlOpenError (errCode, "license", DistributorPurchaseLink);
			TheOutput.Display (msg.c_str (), Out::Error, GetWindow ());
		}
	}
}

void ProjectMembersCtrl::GetSelectedMemberState (MemberState & state)
{
	if (_admin.IsEnabled () && _admin.IsChecked ())
		state.MakeAdmin ();
	else if (_voting.IsEnabled () && _voting.IsChecked ())
		state.MakeVotingMember ();
	else if (_observer.IsEnabled () && _observer.IsChecked ())
		state.MakeObserver ();
	else if (_removed.IsEnabled () && _removed.IsChecked ())
		state.MakeDead ();

	if (_checkoutNotification.IsEnabled ())
		state.SetCheckoutNotification (_checkoutNotification.IsChecked ());
}

// Returns true when member state change is legal
bool ProjectMembersCtrl::VerifyMemberStateChange (MemberState const & oldState, MemberState const & newState)
{
	if (_dlgData.IsAdmin (_dlgData.GetThisUserIdx ()))
	{
		// Project administrator changes his or somebody's else state
		if (_dlgData.IsThisUser (_selectedMemberIdx))
		{
			if (oldState.IsAdmin () && !newState.IsAdmin ())
			{
				// Current administrator state change
				// Inform the user that first he/she has to select new administrator
				std::string info ("Select new project administrator and ");
				if (newState.IsVoting ())
					info += "your state will automatically be changed to voting member";
				else
					info += "then change your state to observer";
				TheOutput.Display (info.c_str ());
				return false;
			}
		}
		else
		{
			// Other project member state change
			if (newState.IsAdmin ())
			{
				// New administrator selection
				unsigned int myIdx = _dlgData.GetThisUserIdx ();
				// Current administrator becomes a voting member if we have enough
				// licensed seats, otherwise current administrator becomes an observer.
				MemberInfo const & myCurrentInfo = _dlgData.GetMemberInfo (myIdx);
				Assert (myCurrentInfo.State ().IsAdmin ());
				if (myCurrentInfo.State ().IsDistributor () && !_distributorPool.empty ())
				{
					// Administrator in distributor project -- explain how to transfer
					// distributor license pool to the new administrator.
					std::string info (_distributorPool.GetLicensesLeftText ());
					info += "\n\nTo transfer the distribution license pool to the new administrator\n"
							"open Dispatcher window and execute Program>Distributor License.";
					TheOutput.Display (info.c_str ());
				}
				MemberInfo myNewInfo(myCurrentInfo);
				bool notEnoughLicenses;
				if (_dlgData.CanMakeVoting (myIdx, notEnoughLicenses))
				{
					StateVotingMember voting (myCurrentInfo.State ());
					myNewInfo.SetState (voting);
				}
				else
				{
					StateObserver observer (myCurrentInfo.State ());
					myNewInfo.SetState (observer);
				}
				_dlgData.SetNewMemberInfo (myIdx, std::move(myNewInfo));
			}
			else if (oldState.IsObserver () && newState.IsVotingMember ())
			{
				// Check if we can change state from observer to voting member
				bool notEnoughSeats;
				if (!_dlgData.CanMakeVoting (_selectedMemberIdx, notEnoughSeats))
				{
					if (notEnoughSeats)
					{
						TheOutput.Display ("You cannot change the status to voting member, because there are\n"
							"not enough licenses in the project.", Out::Information, GetWindow ());
					}
					else
					{
						TheOutput.Display ("You cannot change the status to voting member, because you don't\n"
							"have a valid user license and your trial period has ended.", Out::Information, GetWindow ());
					}
					return false;
				}
			}
		}
	}
	else if (_dlgData.IsThisUser (_selectedMemberIdx)) 
	{
		// Regular project member changes his state
		if (!oldState.IsVoting () && newState.IsVoting ())
		{
			bool notEnoughSeats;
			// Check if we can change state from observer to voting member
			if (!_dlgData.CanMakeVoting (_selectedMemberIdx, notEnoughSeats))
			{
				if (notEnoughSeats)
				{
					TheOutput.Display ("You cannot change the status to voting member, because there are\n"
						"not enough licenses in the project.", Out::Information, GetWindow ());
				}
				else
				{
					TheOutput.Display ("You cannot change the status to voting member, because you don't\n"
						"have a valid user license and your trial period has ended.", Out::Information, GetWindow ());
				}
				return false;
			}
		}
	}
	else
	{
		// Regular user changes unverified user state
		Assert (!newState.IsVerified ());
		if (_dlgData.IsAdmin (_selectedMemberIdx))
		{
			// Unverified user is project admin
			std::string info ("In order to ");
			if (newState.IsDead ())
				info += "remove";
			else
				info += "change the state of";
			info += " the current project administrator";
			if (newState.IsDead ())
				info += " from the project";
			info += '\n';
			info += "you have to first elect the new project administrator (Project>Administrator)\n";
			info += "and then change the user ";
			MemberInfo const & selectedInfo = _dlgData.GetMemberInfo (_selectedMemberIdx);
			MemberNameTag tag (selectedInfo.Name (), selectedInfo.Id ());
			info += tag;
			TheOutput.Display (info.c_str (), Out::Information, GetWindow ()); 
			return false;
		}
	}
	return true;
}

void ProjectMembersCtrl::DisableLicense ()
{
	_licenseDisplay.SetText ("Licensing status unknown.");
	_licensePool.SetText ("");
	_assignText.SetText ("");
	_assignLicense.Hide ();
}

void ProjectMembersCtrl::ShowLicense (int idx)
{
	MemberInfo const & member = _dlgData.GetMemberInfo (idx);
	MemberDescription const & description = member.Description ();
	License license (description.GetLicense ());
	std::string assignText;
	std::string buttonText;
	_licensePool.SetText ("");
	if (_dlgData.ThisUserState ().IsReceiver ())
	{
		// Receiver can only view his own license
		Assert (_dlgData.IsThisUser (idx));
		if (license.IsValid ())
			_licenseDisplay.SetText ("License assigned by Distributor");
		else
			_licenseDisplay.SetText ("Ask the administrator to assign a Receiver License to you");
	}
	else if (_dlgData.IsReceiver (idx) 
		  && _dlgData.ThisUserState ().IsAdmin ()
		  && !license.IsValid ())
	{
		_licensePool.SetText (_distributorPool.GetLicensesLeftText ().c_str ());
		if (!_distributorPool.empty ())
		{
			_licenseDisplay.SetText ("Receiver without a valid license. As an admin, you can assign licenses.");
			assignText = "Use next license from the pool";
			buttonText = "Assign to this receiver";
		}
		else
		{
			_licenseDisplay.SetText ("Receiver without a valid license");
			assignText = "Buy licenses on the WEB:";
			buttonText = "www.relisoft.com";
		}
	}
	else if (license.IsValid ())
	{
		// Selected user has a valid license
		_licenseDisplay.SetText (license.GetDisplayString ().c_str ());
	}
	else if (_dlgData.GetThisUserIdx () == idx)
	{
		// This user has been selected and he doesn't have a valid license
		if (_dlgData.GetTrialDaysLeft () > 0)
		{
			std::string msg ("Evaluation copy -- ");
			msg += ToString (_dlgData.GetTrialDaysLeft ());
			msg += " trial day(s) left.";
			_licenseDisplay.SetText (msg.c_str ());
		}
		else
		{
			_licenseDisplay.SetText ("Expired evaluation copy");
		}
	}
	else
	{
		// Some other user has been selected and he doesn't have a valid license
		_licenseDisplay.SetText ("User without a valid license.");
	}

	if (_dlgData.IsThisUser (idx) && _dlgData.ThisUserState ().IsDistributor () && _dlgData.ThisUserState ().IsAdmin ())
	{
		_licensePool.SetText (_distributorPool.GetLicensesLeftText ().c_str ());
		assignText = "Buy licenses on the WEB:";
		buttonText = "www.relisoft.com";
	}

	_assignText.SetText (assignText.c_str ());
	if (!buttonText.empty ())
	{
		_assignLicense.SetText (buttonText.c_str ());
		_assignLicense.Show ();
	}
	else
		_assignLicense.Hide ();
}

void ProjectMembersCtrl::ApplyChanges ()
{
	if (_selectedMemberIdx == -1)
		return;

	// Determine new member state
	MemberState oldState = _dlgData.GetMemberInfo (_selectedMemberIdx).State ();
    MemberState newState (oldState);
	GetSelectedMemberState (newState);
	if (!newState.IsEqual (oldState))
	{
		if (!VerifyMemberStateChange (oldState, newState))
		{
			// Illegal member state change
			// Remove changes from edit controls and set focus to dialog
			LoadEditControls (_selectedMemberIdx);
			_applyChanges.Disable ();
			GetWindow ().SetFocus ();
			return;
		}
	}
	Assert ((oldState.IsVerified () == newState.IsVerified ()) &&
			(oldState.IsReceiver () == newState.IsReceiver ()) &&
			(oldState.IsDistributor () == newState.IsDistributor ()) &&
			(oldState.NoBranching () == newState.NoBranching ()));
	MemberInfo const & member = _dlgData.GetMemberInfo (_selectedMemberIdx);
	// Check if anything changed
	std::string newName (_name.GetString ());
	std::string newHubId (_hubId.GetString ());
	if (newName.empty () || newHubId.empty ())
	{
		if (newName.empty ())
			TheOutput.Display ("You cannot remove member name.");
		else
			TheOutput.Display ("You cannot remove member hub's email address.");

		LoadEditControls (_selectedMemberIdx);
		_applyChanges.Disable ();
		GetWindow ().SetFocus ();
		return;
	}

	std::string newLicense (_newLicense);
	if (newLicense.empty ())
		newLicense = member.License ();
	
	std::string newComment (_phone.GetString ());
    if (!newState.IsEqual (oldState) || member.Name () != newName ||
		member.HubId () != newHubId || member.Comment () != newComment
		|| member.License () != newLicense)
	{
		// Changes detected
		if (_selectedMemberIdx == _dlgData.GetThisUserIdx ())
		{
			std::string catalogHubId (_catalog.GetHubId ());
			if (!catalogHubId.empty () && !IsNocaseEqual (newHubId, catalogHubId))
			{
				if (member.IsDistributor ())
				{
					if (TheOutput.Prompt ("Are you sure you want to have a different email address "
						" for this project?\n\n"
						"(You can use Dispatcher>Collaboration Settings\n"
						"to change the address (hub id) for all projects on this machine.)",
						Out::OkCancel, GetWindow ()) != Out::OK)
						return;
				}
				else
				{
					// The user hub's email address can only be changed to the catalog hub's email address, if catalog Hub's Email Address is not empty
					std::string msg ("You cannot change your hub's email address from '");
					msg += member.HubId ();
					msg += "' to '";
					msg += newHubId;
					msg += "'.\nYour hub's email address on this machine is: '";
					msg += catalogHubId;
					msg += "'.\nTo change it, you must re-configure the Dispatcher.";
					TheOutput.Display (msg.c_str ());
					LoadEditControls (_selectedMemberIdx);
					_applyChanges.Disable ();
					GetWindow ().SetFocus ();
					return;
				}
			}
		}
		MemberDescription newDescription (newName,
										  newHubId,
										  newComment,
										  newLicense,
										  member.GetUserId ());
		MemberInfo newInfo( member.Id (),
						    newState,
							std::move(newDescription));

		_dlgData.SetNewMemberInfo (_selectedMemberIdx, std::move(newInfo));
		_displayList.Refresh ();
		EnableEditControls ();
	}
}

void ProjectMembersCtrl::Select (int dataIndex)
{
	// Check if user can edit this member
	_selectedMemberIdx = dataIndex;
	LoadEditControls (dataIndex);
	if (_dlgData.CanEditMember (dataIndex))
	{
		// User can edit -- enable controls
		EnableEditControls ();
	}
	else
	{
		// User cannot edit -- disable controls
		DisableEditControls ();
	}
    _applyChanges.Disable ();
}

void ProjectMembersCtrl::LoadEditControls (int dataIndex)
{
	MemberInfo const & member = _dlgData.GetMemberInfo (dataIndex);
	MemberDescription const & description = member.Description ();
	_name.SetString (description.GetName ().c_str ());
	_hubId.SetString (description.GetHubId ().c_str ());
	_phone.SetString (description.GetComment ().c_str ());
	MemberState const state = member.State ();

	if (_dlgData.CanDisplayLicense (dataIndex))
		ShowLicense (dataIndex);
	else
		DisableLicense ();

	_admin.UnCheck ();
	_voting.UnCheck ();
	_observer.UnCheck ();
	_removed.UnCheck ();
	_checkoutNotification.UnCheck ();

	if (state.IsAdmin ())
		_admin.Check ();
	else if (state.IsVotingMember ())
		_voting.Check ();
	else if (state.IsObserver ())
		_observer.Check ();
	else
	{
		Assert (state.IsDead ());
		_removed.Check ();
	}

	if (state.IsDistributor ())
		_distributor.SetText ("Distributor");
	else if (state.IsReceiver ())
		_distributor.SetText ("Receiver");
	else
		_distributor.SetText ("");

	if (state.IsCheckoutNotification ())
		_checkoutNotification.Check ();
}

void ProjectMembersCtrl::EnableEditControls ()
{
    _name.SetReadonly (false);
    _hubId.SetReadonly (false);
    _phone.SetReadonly (false);
	
	if (!_dlgData.IsAdmin (_dlgData.GetThisUserIdx ()) && !_dlgData.IsThisUser (_selectedMemberIdx))
	{
		// Regular project member edits unverified project member -- allow only state change
		Assert (!_dlgData.IsReceiver (_selectedMemberIdx));
		DisableEditControls ();
	}

	std::set<std::string> allowedStates;
	GetAllowedStateChanges (allowedStates);
	_admin.Enable (IsIn ("admin", allowedStates));
	_voting.Enable (IsIn ("voting", allowedStates));
	_observer.Enable (IsIn ("observer", allowedStates));
	_removed.Enable (IsIn ("removed", allowedStates));
	_checkoutNotification.Enable ();
}

void ProjectMembersCtrl::DisableEditControls ()
{
    _name.SetReadonly (true);
    _hubId.SetReadonly (true);
    _phone.SetReadonly (true);
    _voting.Disable ();
    _observer.Disable ();
    _removed.Disable ();
	_admin.Disable ();
	_checkoutNotification.Disable ();
}
