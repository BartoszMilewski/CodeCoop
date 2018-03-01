//---------------------------
// (c) Reliable Software 2005
//---------------------------

#include "precompiled.h"
#include "DistributorTransferDlg.h"
#include "resource.h"

DistributorTransferCtrl::DistributorTransferCtrl (DistributorTransferData & data)
	: Dialog::ControlHandler (IDD_DISTRIB_TRANSFER),
	  _data (data)
{}

bool DistributorTransferCtrl::OnInitDialog () throw (Win::Exception)
{
	_countLeft.Init (GetWindow (), IDC_EDIT1);
	_countLeft.SetString (ToString (_data._countLeft));
	_licensee.Init (GetWindow (), IDC_EDIT2);
	_licensee.SetString (_data._licensee);
	_hubId.Init (GetWindow (), IDC_COMBO);
	_localSave.Init (GetWindow (), IDC_CHECK);
	for (NocaseSet::const_iterator it = _data._hubs.begin (); it != _data._hubs.end (); ++it)
	{
		_hubId.AddToList (*it);
	}
	_hubId.SetSelection (0);
	return true;
}

bool DistributorTransferCtrl::OnDlgControl (unsigned ctrlId, unsigned notifyCode) throw (Win::Exception)
{
	if (ctrlId == IDC_CHECK)
	{
		if (_localSave.IsChecked ())
			_hubId.Disable ();
		else
			_hubId.Enable ();
	}
	return false;
}

bool DistributorTransferCtrl::OnApply () throw ()
{
	_data._targetHub = _hubId.RetrieveEditText ();
	_data._localSave = _localSave.IsChecked ();
	EndOk ();
	return true;
}
