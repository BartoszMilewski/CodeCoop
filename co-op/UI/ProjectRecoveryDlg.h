#if !defined (PROJECTRECOVERYDLG_H)
#define PROJECTRECOVERYDLG_H
//---------------------------
// (c) Reliable Software 2007
//---------------------------

#include "MemberInfo.h"

#include <Ctrl/ListView.h>
#include <Win/Dialog.h>
#include <Ctrl/Static.h>
#include <Ctrl/Button.h>
#include <auto_vector.h>

namespace Project
{
	class Db;
}

class ProjectRecoveryData
{
public:
	ProjectRecoveryData (Project::Db const & projectDb,
						 std::string const & caption,
						 bool isRestoreFromBackup = false);

	bool AmITheOnlyVotingMember () const { return _votingMembers.size () == 0; }
	bool IsAlwaysSendToAdmin () const { return _alwaysSendToAdmin; }
	bool IsRestoreFromBackup () const { return _isRestoreFromBackup; }

	typedef std::vector<MemberInfo>::const_iterator RecipientIter;
	RecipientIter RecipientInfoBegin () const { return _votingMembers.begin (); }
	RecipientIter RecipientInfoEnd () const { return _votingMembers.end (); }
	int RecipientCount () const { return _votingMembers.size (); }

	UserId GetAdminId () const { return _adminId; }
	UserId GetSelectedRecipientId () const { return _selectedRecipientId; }
	bool IsDontBlockCheckin () const { return _dontBlockCheckin; }
	std::string const & GetCaption () const { return _caption; }
	void SelectRecipient (unsigned idx);
	void ClearSelection() { _selectedRecipientId = gidInvalid; }
	void SetDontBlockCheckin (bool flag) { _dontBlockCheckin = flag; }
	void SetAlwaysSendToAdmin (bool flag) { _alwaysSendToAdmin = flag; }

private:
	std::vector<MemberInfo>	_votingMembers;
	UserId					_adminId;
	UserId					_selectedRecipientId;
	bool					_dontBlockCheckin;
	bool					_alwaysSendToAdmin;
	bool					_isRestoreFromBackup;
	std::string				_caption;
};

class ProjectRecoveryRecipientsCtrl;

class RecipientListHandler : public Notify::ListViewHandler
{
public:
	RecipientListHandler (unsigned ctrlId, 
						  ProjectRecoveryData & dlgData, 
						  ProjectRecoveryRecipientsCtrl & ctrl, 
						  std::vector<int> const & listIndex2Data);

	bool OnDblClick () throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();

private:
	ProjectRecoveryRecipientsCtrl &	_ctrl;
	ProjectRecoveryData &			_dlgData;
	std::vector<int> const &		_listIndex2Data;
};

class ProjectRecoveryRecipientsCtrl : public Dialog::ControlHandler
{
public:
	ProjectRecoveryRecipientsCtrl (ProjectRecoveryData & data);

	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void UpdateListView ();

private:
	Win::ReportListing		_recipientList;
	Win::StaticText			_caption;
	Win::CheckBox			_optionCheckbox;
	std::vector<int> 		_listIndex2Data;
	ProjectRecoveryData &	_dlgData;
	RecipientListHandler	_notifyHandler;
};

#endif
