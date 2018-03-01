// ----------------------------------
// (c) Reliable Software, 2001 - 2005
// ----------------------------------

#include "precompiled.h"
#include "PurgeDlg.h"
#include "resource.h"

PurgeCtrl::PurgeCtrl (PurgeData & data)
	: Dialog::ControlHandler (IDD_PURGE),
	  _data (data)
{}

bool PurgeCtrl::OnInitDialog () throw (Win::Exception)
{
	_local.Init (GetWindow (), IDC_LOCAL);
	_sat.Init (GetWindow (), IDC_SAT);
	
	if (_data.IsPurgeLocal ())
		_local.Check ();
	if (_data.IsPurgeSatellite ())
		_sat.Check ();

	GetWindow ().CenterOverOwner ();
	return true;
}

bool PurgeCtrl::OnApply () throw ()
{
	_data.SetPurgeLocal (_local.IsChecked ());
	_data.SetPurgeSatellite (_sat.IsChecked ());
    EndOk ();
    return true;
}
