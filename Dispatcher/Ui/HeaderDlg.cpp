// ----------------------------------
// (c) Reliable Software, 1999 - 2005
// ----------------------------------

#include "precompiled.h"
#include "HeaderDlg.h"
#include "resource.h"
#include "UserIdPack.h"

#include <algorithm>

HeaderCtrl::HeaderCtrl (TransportData const & data, std::string const & filename)
	: Dialog::ControlHandler (IDD_HEADER),
	  _data (data),
	  _scriptFilename (filename)
{}

bool HeaderCtrl::OnInitDialog () throw (Win::Exception)
{
	_filename.Init (GetWindow (), IDC_FILENAME);
	_project.Init (GetWindow (), IDC_PROJECT);
	_hubId.Init (GetWindow (), IDC_EMAIL);
	_userId.Init (GetWindow (), IDC_LOCATION);
	_recipients.Init (GetWindow (), IDC_RECIPIENTS);
	_status.Init (GetWindow (), IDC_STATUS);
	_comment.Init (GetWindow (), IDC_COMMENT);
	_forward.Init (GetWindow (), IDC_FORWARD);
	_defect.Init (GetWindow (), IDC_DEFECT);

	_filename.SetText (_scriptFilename.c_str ());
    _project.SetString (_data.GetProjectName ());
    _hubId.SetString (_data.GetSenderAddress ().GetHubId ());
	UserIdPack pack (_data.GetSenderAddress ().GetUserId ());
    _userId.SetString (pack.GetUserIdString ());
	_status.SetString (_data.GetStatus ().c_str ());
    _comment.SetString (_data.GetComment ().c_str ());
    if (_data.ToBeForwarded ())
        _forward.Check ();
    if (_data.IsDefectScript ())
        _defect.Check ();
    _recipients.AddProportionalColumn (49, "HubId");
    _recipients.AddProportionalColumn (25, "User id");
    _recipients.AddProportionalColumn (21, "Status");
	AddresseeList const & recipients = _data.GetRecipients ();
    std::for_each (recipients.begin (), recipients.end (), AddRow (_recipients));
	return true;
}

bool HeaderCtrl::OnApply () throw ()
{
	EndOk ();
	return true;
}
