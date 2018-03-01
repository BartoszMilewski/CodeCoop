//----------------------------------
//  (c) Reliable Software, 2007
//----------------------------------

#include "precompiled.h"
#include "FilePropertiesDlg.h"
#include "FileProps.h"
#include "CheckoutNotifications.h"
#include "Projectdb.h"
#include "MemberDescription.h"
#include "Resource.h"

CheckedOutByPageHndlr::CheckedOutByPageHndlr (FileProps & dlgData)
	: PropPage::Handler (IDD_CHECKEDOUT_BY),
	  _dlgData (dlgData)
{}

bool CheckedOutByPageHndlr::OnInitDialog () throw (Win::Exception)
{
	_memberList.Init (GetWindow (), IDC_MEMBER_LIST);
	_startNotifications.Init (GetWindow (), IDC_START_CHECKOUT_NOTIFICATIONS);
	for (FileProps::CheckoutSequencer seq (_dlgData); !seq.AtEnd (); seq.Advance ())
	{
		_memberList.AddItem (seq.GetMember ().c_str ());
	}
	if (_dlgData.IsCheckoutNotificationOn ())
		_startNotifications.Check ();
	return true;
}

void CheckedOutByPageHndlr::OnApply (long & result) throw (Win::Exception)
{
	result = 0;	// Assume everything is ok
	_dlgData.SetCheckoutNotifications (_startNotifications.IsChecked ());
}

FilePropertyHndlrSet::FilePropertyHndlrSet (FileProps & props)
	: PropPage::HandlerSet (props.GetCaption ()),
	  _checkedOutByPageHndlr (props)
{
	AddHandler (_checkedOutByPageHndlr, "Checked out by");
}
