#if !defined (OVERDUESCRIPTDLG_H)
#define OVERDUESCRIPTDLG_H
//------------------------------------
//  (c) Reliable Software, 2004 - 2005
//------------------------------------
#include "GlobalId.h"
#include <Win/Dialog.h>
#include <Ctrl/ListView.h>
#include <Ctrl/Button.h>

class ScriptProps;

class OverdueScriptCtrl : public Dialog::ControlHandler, Notify::ListViewHandler
{
public:
    OverdueScriptCtrl (ScriptProps const & data, std::set<UserId> & membersToRemove, bool & showDetails);

    bool OnInitDialog () throw (Win::Exception);
	bool OnDlgControl (unsigned id, unsigned notifyCode) throw (Win::Exception);
	Notify::Handler * GetNotifyHandler (Win::Dow::Handle winFrom, unsigned idFrom) throw ()
	{
		return this;
	}
	// Notify::ListViewHandler
	bool OnItemChanged (Win::ListView::ItemState & state) throw ();
private:
	Win::ReportListing	_memberListing;
	Win::Button			_removeButton;
    ScriptProps const &	_dlgData;
	std::set<UserId> & _membersToRemove;
	bool & _showDetails;
};

#endif
