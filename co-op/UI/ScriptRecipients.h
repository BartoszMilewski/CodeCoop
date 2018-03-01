#if !defined (SCRIPTRECIPIENTS_H)
#define SCRIPTRECIPIENTS_H
//------------------------------------
//	(c) Reliable Software, 1998 - 2008
//------------------------------------

#include "MemberInfo.h"
#include "Lineage.h" // Unit::Type

#include <Ctrl/ListView.h>
#include <Ctrl/Button.h>
#include <Win/Dialog.h>

#include <auto_vector.h>

class AddresseeList;

class ScriptRecipientsData
{
public:
	ScriptRecipientsData (std::vector<MemberInfo> & addresseeList, GlobalId adminId);

	typedef std::vector<MemberInfo>::const_iterator recipientIter;
	recipientIter RecipientInfoBegin () const { return _addresseeList.begin (); }
	recipientIter RecipientInfoEnd () const { return _addresseeList.end (); }
	int RecipientCount () const { return _addresseeList.size (); }
	void Select (int dataIndex, bool wasSelected, bool isSelected);
	void SelectAll ();
	void DeselectAll ();

	bool IsSelection () const;
	void GetSelection (AddresseeList & addresseeList) const;
	void GetSelection (UserIdList & addresseeList) const;
	bool HasScriptId () const
	{
		return _scriptId != gidInvalid;
	}
	GlobalId GetScriptId ()
	{
		return _scriptId;
	}
	void SetScriptId (GlobalId id)
	{
		_scriptId = id;
	}
	GlobalId GetAdminId () const { return _adminId; }
	Unit::Type GetUnitType () const { return _unitType; }
	void SetUnitType (Unit::Type type)
	{
		_unitType = type;
	}
	void SelectRecipient (UserId id);
private:
	std::vector<MemberInfo>   & _addresseeList;
	std::vector<bool>			_selected;
	GlobalId					_adminId;
	GlobalId					_scriptId;
	Unit::Type					_unitType;
};

// Windows WM_NOTIFY handlers

class ScriptRecipientsCtrl;

class RecipientHandler : public Notify::ListViewHandler
{
public:
	RecipientHandler (unsigned ctrlId, 
					  ScriptRecipientsData * dlgData, 
					  ScriptRecipientsCtrl & ctrl, 
					  std::vector<int> const & listIndex2Data);

	bool OnDblClick () throw ();
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();

private:
	ScriptRecipientsCtrl &		_ctrl;
	ScriptRecipientsData *		_dlgData;
	std::vector<int> const & 	_listIndex2Data;
};

class ScriptRecipientsCtrl : public Dialog::ControlHandler
{
public:
	ScriptRecipientsCtrl (ScriptRecipientsData * data);
	bool GetDataFrom (NamedValues const & source);

	void EndDialog () { EndOk (); }
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ();
	bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception);
	bool OnApply () throw ();

private:
	void UpdateListView ();

private:
	Win::ReportListing		_recipientList;
	std::vector<int> 		_listIndex2Data;
	Win::Button 			_selectAll;
	Win::Button 			_deselectAll;
	ScriptRecipientsData *	_dlgData;
	RecipientHandler		_notifyHandler;
};

#endif
