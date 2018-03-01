// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "precompiled.h"
#include "HubTransportDlg.h"
#include "OutputSink.h"

#include "resource.h"

HubTransportCtrl::HubTransportCtrl (std::string const & hubId)
	: Dialog::ControlHandler (IDD_HUBTRANSPORT),
	  _hubId (hubId)
{}

bool HubTransportCtrl::OnInitDialog () throw (Win::Exception)
{
	_readEdit.Init (GetWindow (), IDC_READEDIT);
	_edit.Init (GetWindow (), IDC_EDIT);
	_readEdit.SetText (_hubId.c_str ());
	return true;
}

bool HubTransportCtrl::OnApply () throw ()
{
	if (_edit.GetLen () == 0)
	{
		TheOutput.Display ("Please specify hub transport (email).");
		return true;
	}
	_transport.Init (_edit.GetString ());
	if (_transport.IsUnknown ())
	{
		TheOutput.Display ("The transport you specified cannot be recognized by the Dispatcher");
		return true;
	}
	EndOk ();
    return true;
}
