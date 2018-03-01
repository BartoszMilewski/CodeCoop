// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "precompiled.h"
#include "RemoteHubTransportDlg.h"
#include "ConfigData.h"
#include "OutputSink.h"
#include "Resource.h"

InterClusterTransportCtrl::InterClusterTransportCtrl (InterClusterTransportData & data)
	: Dialog::ControlHandler (IDD_REMOTE_HUB_TRANSPORT),
	  _dlgData (data)
{}

bool InterClusterTransportCtrl::OnInitDialog () throw (Win::Exception)
{ 
	_hubIdEdit.Init (GetWindow (), IDC_HUB_ID);
	_transportEdit.Init (GetWindow (), IDC_TRANSPORT);
	_transportTypeEdit.Init (GetWindow (), IDC_TRANSPORT_TYPE);

	_hubIdEdit.SetString (_dlgData.GetHubId ().c_str ());
	_transportEdit.SetString  (_dlgData.GetOldTransport ().GetRoute ().c_str ());
	std::string type;
	switch (_dlgData.GetOldTransport ().GetMethod ())
	{
	case Transport::Network:
		type = "Network";
	case Transport::Email:
		type = "E-mail";
		break;
	default:
		type = "Unknown";
	};
	_transportTypeEdit.SetString  (type.c_str ());
	return true;
}

bool InterClusterTransportCtrl::OnApply () throw ()
{
	Transport transport (_transportEdit.GetString ());
	bool validTransport = TheTransportValidator->ValidateRemoteHub (transport,
																	GetWindow ());
	if (validTransport)
	{
		_dlgData.SetNewTransport (transport);
		EndOk ();
	}
	return true;
}
