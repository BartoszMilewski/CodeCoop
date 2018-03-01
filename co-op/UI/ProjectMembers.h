#if !defined PROJECTMEMBERS_H
#define PROJECTMEMBERS_H

//----------------------------------
// (c) Reliable Software 1997 - 2008
//----------------------------------

#include "MemberInfo.h"
#include "DistributorLicense.h"

#include <Ctrl/ListView.h>
#include <Ctrl/Edit.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>
#include <auto_vector.h>

namespace Project
{
	class Db;
}
class Catalog;
class ActivityLog;

//
// Project Members dialog
//

class ProjectMembersData
{
public:
	ProjectMembersData (Project::Db const & projectDb, unsigned int trialDaysLeft);

	typedef std::vector<MemberInfo>::const_iterator memberIter;
	memberIter MemberInfoBegin () const { return _memberData.begin (); }
	memberIter MemberInfoEnd () const { return _memberData.end (); }
	unsigned int MemberCount () const { return _memberData.size (); }
	unsigned int FullMemberCount () const;

	int CmpRows (int rowX, int rowY, int sortCol) const;
	bool CanEditMember (unsigned int dataIndex) const;
	bool CanMakeVoting (int selectedMember, bool & notEnoughSeats) const;
	bool IsThisUser (unsigned int dataIndex) const { return _thisUserIdx == dataIndex; }
	bool IsAdmin (unsigned int dataIndex) const { return _memberData [dataIndex].State ().IsAdmin (); }
	bool IsReceiver (unsigned int dataIndex) const { return _memberData [dataIndex].State ().IsReceiver (); }
	bool ChangesDetected () const;

	UserId GetThisUserId () const { return _memberData [_thisUserIdx].Id (); }
	MemberState ThisUserState () const { return _thisUserState; }
	unsigned int GetThisUserIdx () const { return _thisUserIdx; }
	unsigned int ElectNewAdmin (unsigned int currentAdminIdx);
	int GetMissingSeats () const { return _missingSeats; }
	int GetTrialDaysLeft () const { return _trialDaysLeft; }

	void SetNewMemberInfo (int index, MemberInfo && newMemberInfo);
	void ChangeMemberState (int index, MemberState newState);
	MemberInfo const & GetMemberInfo (int index) const { return _memberData [index]; }
	bool CanDisplayLicense (int idx);

public:
	class Sequencer
	{
	public:
		Sequencer (ProjectMembersData const & dlgData)
			: _cur (0),
			  _member (dlgData._memberData)
		{
			for (size_t i = 0; i < dlgData._memberData.size (); i++)
			{
				if (dlgData._infoChanged [i] == 1)
					_edited.push_back (i);
			}
		}
		bool AtEnd () const { return _cur == _edited.size (); }
		void Advance () { _cur++; }
		MemberInfo const & GetMemberInfo () const { return _member [_edited[_cur]]; }

	private:
		unsigned int					_cur;
		std::vector<int>				_edited;
		std::vector<MemberInfo> const & _member;
	};

	friend class Sequencer;

private:
	int CountMissingSeats () const;

private:
	unsigned int			_thisUserIdx;
	MemberState				_thisUserState;
	int 					_trialDaysLeft;
	int 					_missingSeats;
	std::vector<MemberInfo> _memberData;
	std::vector<int>		_infoChanged;
};

class ProjectMembersCtrl;

class MemberDisplay : public Notify::ListViewHandler
{
public:
	MemberDisplay (ProjectMembersData const & memberList,
				   ProjectMembersCtrl & dlgCtrl,
				   Win::Dow::Handle winParent);
	~MemberDisplay ();

	void Init (Win::Dow::Handle dlgWin, unsigned ctrlId);
	void Refresh ();

	// Notify::ListViewHandler
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
	bool OnColumnClick (int col) throw ();

private:
	enum
	{
		imageOverlayAdministrator,
		imageMember,
		imageThisMember,
		imageVerifiedMember,
		imageThisVerifiedMember,
		imageLast
	};

	enum
	{
		overlayAdministrator = 1,
		overlayThisMember = 2
	};

	enum
	{
		colName,
		colHubId,
		colState,
		colId
	};

private:
	class SortPredicate
	{
	public:
		SortPredicate (ProjectMembersData const & memberList, int sortCol, bool isAscending)
			: _memberList (memberList),
			  _sortCol (sortCol),
			  _isAscending (isAscending)
		{}

		bool operator ()(int rowX, int rowY);

	private:
		int							_sortCol;
		bool						_isAscending;
		ProjectMembersData const &	_memberList;
	};

private:
	static int const			_iconIds [imageLast];
	ImageList::AutoHandle		_memberImages;
	Win::ReportListing			_displayList;
	int							_sortColumn;
	bool						_isAscending;	// True - ascending; false - descending
	ProjectMembersData const &	_memberList;
	int							_selectedMemberIdx;
	ProjectMembersCtrl &		_dlgCtrl;
	std::vector<int>			_sortVector;
};

//----------------------------------
// Project members dialog controller
//----------------------------------

class ProjectMembersCtrl : public Dialog::ControlHandler
{
	friend class MemberDisplay;

public:
	ProjectMembersCtrl (ProjectMembersData & data, 
						Catalog & catalog,
						ActivityLog & log,
						Win::Dow::Handle winParent);

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

	bool GetDataFrom (NamedValues const & source);

private:
	void GetSelectedMemberState (MemberState & state);
	bool VerifyMemberStateChange (MemberState const & oldState, MemberState const & newState);
	void GetAllowedStateChanges (std::set<std::string> & allowedStates) const;
	void DisableLicense ();
	void ShowLicense (int idx);
	void ApplyChanges ();
	void Select (int dataIndex);
	void LoadEditControls (int dataIndex);
	void EnableEditControls ();
	void DisableEditControls ();
	void AssignReceiverLicense ();

private:
	Catalog	&				_catalog;
	ActivityLog	&			_log;
	DistributorLicensePool	_distributorPool;
	MemberDisplay			_displayList;
	Win::Edit				_name;
	Win::Edit				_hubId;
	Win::Edit				_phone;
	Win::RadioButton		_voting;
	Win::RadioButton		_observer;
	Win::RadioButton		_removed;
	Win::RadioButton		_admin;
	Win::CheckBox			_checkoutNotification;
	Win::StaticText 		_distributor;
	Win::StaticText			_licenseDisplay;
	Win::StaticText			_licensePool;
	Win::StaticText			_assignText;
	Win::Button		 		_assignLicense;
	Win::Button 			_applyChanges;
	Win::Button				_cancel;
	std::string				_newLicense;
	ProjectMembersData &	_dlgData;
	int						_selectedMemberIdx;
};

#endif
