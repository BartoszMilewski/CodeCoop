// ----------------------------------
// (c) Reliable Software, 2002 - 2005
// ----------------------------------

#include "precompiled.h"
#include "HubIdDlg.h"
#include "OutputSink.h"

#include "resource.h"

HubIdCtrl::HubIdCtrl (std::vector<std::string> const & emailList, std::string & hubId, bool isHub)
	: Dialog::ControlHandler (isHub ? IDD_HUB_ID_HUB : IDD_HUB_ID_SAT),
	  _emailList (emailList),
	  _hubId (hubId)
{}

bool HubIdCtrl::OnInitDialog () throw (Win::Exception)
{
	_combo.Init (GetWindow (), IDC_HUB_ID);
	if (_emailList.empty ())
		return true;

	for (std::vector<std::string>::const_iterator it = _emailList.begin ();
		 it != _emailList.end ();
		 ++it)
	{
		_combo.AddToList (it->c_str ());
	}
	_combo.SetSelection (0);
	return true;
}

bool HubIdCtrl::OnApply () throw ()
{
	_hubId = _combo.RetrieveEditText ();
	if (_hubId.empty ())
	{
		TheOutput.Display ("Please specify the hub identifier.");
	}
	else
	{
		EndOk ();
	}
    return true;
}
